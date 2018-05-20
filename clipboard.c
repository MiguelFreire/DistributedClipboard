#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "clipboard.h"
#include "cbmessage.pb-c.h"
#include "utils.h"

/*REMOTE Clipbaord Linked Lists*/

typedef struct cb_client {
    size_t id;
    pthread_t thread_id;
    size_t socket_fd;
    size_t socket_teste;
    struct cb_client *next;
} cb_client; 

typedef struct connected_list {
    cb_client *cb;
    size_t size;
} connected_list;

connected_list *new_list() {
    connected_list *list;
    list = scalloc(1,sizeof(connected_list)); //Init all values to default

    return list;
}

cb_client *new_clipboard(size_t socket_fd, size_t socket_teste) {
    //TODO: validate arguments
    cb_client *cb;

    cb = scalloc(1,sizeof(cb_client));
    cb->socket_fd = socket_fd;
    cb->socket_teste = socket_teste;
    return cb;
}

void add_clipboard(connected_list *list, cb_client *cb) {
    cb_client *aux;

    if(list->cb == NULL) { //If no head exists add it to the head
       list->cb = cb;
       list->size++;
       cb->id = list->size;
       return;
   }

   aux = list->cb;
   while(aux->next != NULL) aux = aux->next; //go to the end of the list
   
    aux->next = cb;
    list->size++;
}

void remove_clipboard_by_thread_id(connected_list *list, pthread_t thread_id) {
    cb_client *aux;
    cb_client *cur;
    bool found = false;
    if(list->cb == NULL) return;
    cur = list->cb;
    //Current After
    //Next To be removed
    //Next next 

    while(cur->next != NULL) {
        if(cur->next->thread_id == thread_id) {
            found = true;
            break;
        }
        cur = cur->next;
    }
    if(found) {
        aux = cur->next; //to be removed
        cur->next = aux->next;

        free(aux);
        list->size--;
     }
   
}

void free_list(connected_list *list) {
    cb_client *aux, *cur;

    cur = list->cb;

    while(cur != NULL) {
        aux = cur;
        cur = aux->next;
        free(aux);
    }

    free(list);
}

/*END REMOTE Clipboard Linked List */

typedef struct store_object {
    void *data;
    size_t size;
} store_object;

/*Threads*/
pthread_t thread_regions[NUM_REGIONS];
pthread_t thread_unix_com_handler;
pthread_t thread_inet_com_handler;
pthread_t thread_upper_com_handler;
pthread_t thread_lower_com_handler;
pthread_t thread_child;
    /*CLIPBOARD VARIABLES*/
bool connected_mode;
char *local_ip;
int local_port;
store_object *store;
/*UNIX_COM VARIABLES */

/*INET_COM VARIABLES*/
char *remote_ip; //remote ip
int remote_port;  //remote port
int socket_fd_inet_local; //socket for inet com
int socket_fd_inet_remote;
int socket_fd_unix;
int socket_fd_inet_child;
struct sockaddr_in local_addr; //
struct sockaddr_in remote_addr; // 
struct sockaddr_un clipboard_addr;
struct sockaddr_un client_addr;

connected_list *cblist;
connected_list *applist;
pthread_rwlock_t rwlocks[NUM_REGIONS];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_cond = PTHREAD_COND_INITIALIZER;

int last_region = -1;
int uport = 0;


packed_message new_sync_message()
{
    CBMessage msg = CBMESSAGE__INIT;

    size_t packed_size;
    void* buffer;
    packed_message package = {NULL, 0};

    msg.type = Request;
    msg.method = Sync;

    msg.data = scalloc(NUM_REGIONS, sizeof(ProtobufCBinaryData));
                                            
    for(int i = 0; i < NUM_REGIONS; i++) {
        msg.n_data++;
        if(store[i].size > 0) {
            msg.data[i].data = store[i].data;
            msg.data[i].len = store[i].size;
        }
    }

    packed_size = cbmessage__get_packed_size(&msg);
   
    buffer = smalloc(packed_size);
    cbmessage__pack(&msg, buffer);

    package.buf = buffer;
    package.size = packed_size;
    
    free(msg.data);

    return package;
}

int cbstore(size_t region, void* data, size_t count) {
   if(!validate_region(region) || data == NULL || store == NULL)
        return -1;

    if(store[region].data != NULL) {
        free(store[region].data);
        store[region].size = 0;
    }

    store[region].data = smalloc(count);
    memcpy(store[region].data, data, count);
    store[region].size = count;

    return 1;
}
int clipboard_sync(int clipboard_id) {
    CBMessage *msg;
    int bytes = 0;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t size;
    void *buffer;
    packed_message response;
    packed_message request = new_message(Request, Sync, 0, NULL,0,0,0,0,0);
    store_object *aux;

    bytes = write(clipboard_id, request.buf, request.size);
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }

    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(msg->has_status && !msg->status) {
        cbmessage__free_unpacked(msg, NULL);
        free(request.buf);
        return 0;
    }

    size = msg->size;
    buffer = smalloc(size);

    cbmessage__free_unpacked(msg, NULL); //lets free the msg to reuse later

    response = new_message(Request, Sync, 0, NULL, 0, 1,1,0,0);

    bytes = write(clipboard_id, response.buf, response.size);

    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    bytes = sread(clipboard_id, buffer, size);
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, size, buffer);

    printf("%d %s\n", msg->data[3].len, msg->data[3].data);

    for(int i = 0; i < NUM_REGIONS; i++) {
        if(msg->data[i].len > 0) {
            store[i].data = smalloc(msg->data[i].len);
            memcpy(store[i].data, msg->data[i].data, msg->data[i].len);
            store[i].size = msg->data[i].len;
        }
    }

    cbmessage__free_unpacked(msg, NULL);

    free(request.buf);
    free(response.buf);

    return size;
}

int handleSync(int client, CBMessage *msg) {
    size_t count;
    void *data_buffer;
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    

    //Create response with data
    response = new_sync_message();
    //Create response with size
    response_with_size = new_message(Response, msg->method, 0, NULL, response.size, 0,0,0,0);

    bytes = write(client, response_with_size.buf, response_with_size.size);

    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(msg->has_status && msg->status) {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if (bytes == -1) {
            logs(strerror(errno), L_ERROR);
            //We should return and error and free stuff! 
        }
    }

    cbmessage__free_unpacked(msg, NULL);

    free(response.buf);

    return 1;
}

int handleCopy(int client, CBMessage *msg) {
    size_t count;
    void *data_buffer;
    packed_message response;
    size_t bytes;

    //Save data size to count
    count = msg->size;
    //Alloc buffer to save data
    data_buffer = smalloc(count); 
    //Tell the client we are ready to receive data
    response = new_message(Response, msg->method, msg->region,NULL,0,1,1,0,0); 
    //Lets clear the message structure to reuse
    cbmessage__free_unpacked(msg,NULL); 
    //Write response to client
    printf("Writing to client...\n");
    bytes = write(client, response.buf, response.size);
    printf("Done Writing\n");
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(data_buffer);
        return 0;
    }

    //Read all bytes (count) from client
    bytes = sread(client, data_buffer, count);
   
    msg = cbmessage__unpack(NULL, count, data_buffer);
    //Store new data in the store, store will be shared var

    pthread_rwlock_wrlock(&rwlocks[msg->region]);
    cbstore(msg->region, msg->data->data, msg->data->len);
    pthread_rwlock_unlock(&rwlocks[msg->region]);


    cbmessage__free_unpacked(msg, NULL);
    free(data_buffer);

    return 1;
}

int handlePaste(int client, CBMessage *msg) {
    size_t count;
    void *data_buffer;
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;

    int region = msg->region;
    printf("REGION: %d\n", region);
    logs("CHECKING", L_INFO);

    /*Read LOCK!*/
    pthread_rwlock_rdlock(&rwlocks[region]);
    if(store[region].data == NULL) {
        logs("Region has no data", L_INFO);
        //there is no data in that region send negative response
        response = new_message(Response, msg->method, msg->region, NULL, 0, 1,0,0,0);
        
        cbmessage__free_unpacked(msg, NULL);
        pthread_rwlock_unlock(&rwlocks[region]);
        bytes = write(client, response.buf, response.size);
        
        free(response.buf);
        
        return 0;
    }

    //Create response with data
    response = new_message(Response, msg->method, region, store[region].data, store[region].size,1,1,0,0);
    /*Read UNLOCK!*/
    pthread_rwlock_unlock(&rwlocks[region]);
    //Create response with size
    response_with_size = new_message(Response, msg->method, region, NULL, response.size, 0,0,0,0);

    bytes = write(client, response_with_size.buf, response_with_size.size);

    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(msg->has_status && msg->status) {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if (bytes == -1) {
            logs(strerror(errno), L_ERROR);
            //We should return and error and free stuff! 
        }
    }

    cbmessage__free_unpacked(msg, NULL);

    free(response.buf);

    return 1;
}

int handleWait(int client, CBMessage *msg)
{
    size_t count;
    void *data_buffer;
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;

    int region = msg->region;
    printf("REGION: %d\n", region);
    logs("CHECKING", L_INFO);

    /*Read LOCK!*/
    pthread_mutex_lock(&wait_mutex);
    pthread_cond_wait(&wait_cond, &wait_mutex);
    pthread_rwlock_rdlock(&rwlocks[region]);

    //Create response with data
    response = new_message(Response, msg->method, region, store[region].data, store[region].size, 1, 1, 0, 0);
    /*Read UNLOCK!*/
    pthread_rwlock_unlock(&rwlocks[region]);
    pthread_mutex_unlock(&wait_mutex);
    //Create response with size
    response_with_size = new_message(Response, msg->method, region, NULL, response.size, 0, 0, 0, 0);

    bytes = write(client, response_with_size.buf, response_with_size.size);

    if (bytes == -1)
    {
        logs(strerror(errno), L_ERROR);
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (msg->has_status && msg->status)
    {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if (bytes == -1)
        {
            logs(strerror(errno), L_ERROR);
            //We should return and error and free stuff!
        }
    }

    cbmessage__free_unpacked(msg, NULL);

    free(response.buf);

    return 1;
}

void usage() {
    printf("Usage: \n");
    printf("\t Single Mode: clipboard \n");
    printf("\t Connected Mode: clipboard -c <ip> <port> \n");
    printf("\t \t ip: ipv4 dot format \n");
    printf("\t \t port: integer \n");
}

void *thread_lower_com(void *arg) {
    int l_region;
    int bytes;
    cb_client *cb;
    logs("Lower Com Thread launched", L_INFO);
    while(1) {
        printf("COND2\n");
        pthread_mutex_lock(&mutex2);
        pthread_cond_wait(&cond2, &mutex2);
        l_region = last_region;
        pthread_rwlock_rdlock(&rwlocks[l_region]);
        printf("Entrou3!!\n");
        cb = cblist->cb;
        while(cb != NULL) {
            printf("Enviando...\n");
            printf("Socket:%d\n", cb->socket_teste);
            bytes = clipboard_lower_copy(cb->socket_teste, l_region, store[l_region].data, store[l_region].size);
            cb = cb->next;
            printf("Enviado para o filho\n");
        }
        pthread_rwlock_unlock(&rwlocks[l_region]);
        printf("Saiu1!!\n");
        pthread_mutex_unlock(&mutex2);
        printf("Saiu2!!\n");
    }
}

void *thread_upper_com(void *arg) {
    int l_region;
    int bytes;
    logs("Thread Upper com handler started!",L_INFO);
    while(1) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        l_region = last_region;
        pthread_rwlock_rdlock(&rwlocks[l_region]);
        bytes = clipboard_copy(socket_fd_inet_remote, l_region, store[l_region].data, store[l_region].size);
        pthread_rwlock_unlock(&rwlocks[l_region]);
        pthread_mutex_unlock(&mutex);
    }
}

void *thread_unix_client(void *arg)  {
    int *client = (int*) arg;
    printf("Client: %d\n", *client);
    CBMessage *msg;
        
    uint8_t size_buffer[MESSAGE_MAX_SIZE];
    int bytes = 0;
    int status;   

    logs("Client connected!", L_INFO);
    while(1) {
        bzero(size_buffer, MESSAGE_MAX_SIZE);
        bytes = read(*client, size_buffer, MESSAGE_MAX_SIZE);
        if(bytes == 0) {
            remove_clipboard_by_thread_id(applist, pthread_self());
            logs("Client removed from client list", L_INFO);
            break;
        }
        if(bytes == -1) {
            logs(strerror(errno), L_ERROR);
        }
        printf("Received %d\n", bytes);
        //Unpacks from proto to c format
        msg = cbmessage__unpack(NULL, bytes, size_buffer);
        status = 0;
        
        switch (msg->method)
        {
            case Copy:
                logs("Handling copy method...", L_INFO);
                status = handleCopy(*client, msg);
                status ? logs("Copy method handled successfuly", L_INFO) : logs("Error handlying copy method", L_ERROR);
                printf("CONNECTED:MODE - %d\n", connected_mode);
                pthread_mutex_lock(&wait_mutex);
                last_region = msg->region;
                pthread_cond_broadcast(&wait_cond);
                pthread_mutex_unlock(&wait_mutex);
                if(connected_mode && status && !msg->lower_copy) {
                    logs("Sending copy call to parent", L_INFO);
                    pthread_mutex_lock(&mutex);
                    last_region = msg->region;
                    pthread_cond_signal(&cond);
                    pthread_mutex_unlock(&mutex);
                } else if((!connected_mode || msg->lower_copy) &&  (cblist->size > 0)) {
                    logs("Sending copy call to children", L_INFO);
                    pthread_mutex_lock(&mutex2);
                    printf("Entrou20!!\n");
                    last_region = msg->region;
                    pthread_cond_broadcast(&cond2);
                    printf("Entrou21!!\n");
                    pthread_mutex_unlock(&mutex2);
                    printf("COM DONE!\n");
                }
                break;
            case Paste:
                logs("Handling paste method...", L_INFO);
                status = handlePaste(*client, msg);
                status ? logs("Paste method handled successfuly", L_INFO) : logs("Error handling paste method", L_ERROR);
                break;
            case Sync:
                logs("Handling sync method...", L_INFO);
                status = handleSync(*client, msg);
                status ? logs("Sync method handled successfuly", L_INFO) : logs("Error handling sync method", L_ERROR);
                break;
            case Wait:
                logs("Handling wait method...", L_INFO);
                status = handleWait(*client, msg);
                status ? logs("Wait methond handled successfuly", L_INFO) : logs("Error handling wait method", L_ERROR);
            default:
                break;
        }

    }
    logs("Client disconnected!", L_INFO);
    logs("Thread terminating!", L_INFO);

    return 0;
}

void *thread_inet_handler(void * arg) {
        unsigned int addr_size = sizeof(client_addr);
        int client;
        int child;        
        cb_client *rcb;

        while(1) {
            logs("Waiting for remote clients...", L_INFO);
            rcb = NULL;
            client = accept(socket_fd_inet_local, (struct sockaddr *) &client_addr, &addr_size);
            child = accept(socket_fd_inet_local, (struct sockaddr *)&client_addr, &addr_size);
            rcb = new_clipboard(client, child);
            add_clipboard(cblist, rcb);
            if(client == -1) {
                logs(strerror(errno), L_ERROR);
                exit(-1);
            }
            pthread_create(&rcb->thread_id, NULL, thread_unix_client, &rcb->socket_fd);
            printf("Client criado com a thread ID %lu e socket %d \n", rcb->thread_id, rcb->socket_fd);
            
        }

    return 0;
}

void configure_unix_com() {
    clipboard_addr.sun_family = AF_UNIX; 
    strcpy(clipboard_addr.sun_path, CLIPBOARD_SOCKET);
    
    //Create socket for local clipboard communication (UNIX DATASTREAM)
    unlink(CLIPBOARD_SOCKET);
    socket_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);

    if(socket_fd_unix == -1) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    } 

    //Bind socket to address

    if(bind(socket_fd_unix, (struct sockaddr *) &clipboard_addr, sizeof(clipboard_addr)) == -1) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    if(listen(socket_fd_unix,1) == -1) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }
}

void configure_inet_local_com() {
    int local_port;
    if (uport == 0) {
        srand(time(NULL)); //Use time as seeder for now, maybe change for getpid?
        local_port = rand() % (64738 - 1024) + 1024;
    } else local_port = uport;
    
    socket_fd_inet_local = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd_inet_local == -1) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    } 

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(socket_fd_inet_local, (struct sockaddr *) &local_addr, sizeof(struct sockaddr_in)) == -1) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }  

    if(listen(socket_fd_inet_local,2) == -1) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    printf("Port:%d\n", local_port);
}

void configure_inet_remote_com() {
    socket_fd_inet_remote = socket(AF_INET,SOCK_STREAM,0);
    socket_fd_inet_child = socket(AF_INET, SOCK_STREAM, 0);
        if(socket_fd_inet_remote == -1 || socket_fd_inet_child == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(remote_port);
        
        if(inet_aton(remote_ip, &remote_addr.sin_addr) == 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

        if (connect(socket_fd_inet_remote, (const struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) == -1 ||
            connect(socket_fd_inet_child, (const struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) == -1)
        {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }
}

int main(int argc, char **argv) {
    
    /*Program Vars*/
    int c; //var for opt
    logs("Store created", L_INFO);
    //TODO: create new store function
    store = scalloc(NUM_REGIONS, sizeof(store_object)); 
    cblist = new_list();
    applist = new_list();

    logs("Creating read-write locks...", L_INFO);
    for(int j = 0; j < NUM_REGIONS; j++) {
        pthread_rwlock_init(&rwlocks[j], NULL); //TODO: validate return
    }


    /*Handle Arguments / Program Options*/
    while((c=getopt(argc, argv, "c:p:")) != -1) {
        switch(c) {
            case 'c':
                remote_ip = optarg;
                remote_port = atoi(argv[optind]); //add arguments checker
                connected_mode = true;
                break;
            case 'p':
                uport = atoi(optarg);
                logs("PORT FORCED BY USER", L_INFO);
                break;
            default:
                logs("No arguments given, launching just in single mode...", L_INFO);
                break;
        }   
    }
    /*Configure UNIX Socket Connection */
    configure_unix_com();
    logs("UNIX Sockets set!", L_INFO);
    /*Configure INET Socket Connection */
    configure_inet_local_com();
    logs("INET Sockets set!", L_INFO);
    if(connected_mode) {
        /*Connect to remote clipboard*/
        configure_inet_remote_com();
        clipboard_sync(socket_fd_inet_remote);
        logs("Sync done!", L_INFO);
        pthread_create(&thread_upper_com_handler, NULL, thread_upper_com, NULL);
        pthread_create(&thread_child, NULL, thread_unix_client, &socket_fd_inet_child);
    }
    pthread_create(&thread_lower_com_handler, NULL, thread_lower_com, NULL);
    //free it in the end!

    //single mode
    logs("Starting in single mode.", L_INFO);
    logs("Creating local clipboard", L_INFO);
    //UNIX SOCKET CONNECTION
    //Create address for local clipboard
    

    //END UNIX SOCKET CONNECTION
    logs("Local server started!", L_INFO); 

    
    unsigned int addr_size = sizeof(client_addr);
    int client;        
    pthread_t id;
    cb_client *rcb;
    
    
    pthread_create(&thread_inet_com_handler, NULL, thread_inet_handler, NULL);

    while(1) {
        logs("Waiting for local clients...", L_INFO);
        client = accept(socket_fd_unix, (struct sockaddr *) &client_addr, &addr_size);
        
        if(client == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }
        rcb = new_clipboard(client,0);
        add_clipboard(applist, rcb);
        pthread_create(&rcb->thread_id, NULL, thread_unix_client, &rcb->socket_fd);

    }
    //TODO: free store and free cblist
    unlink(CLIPBOARD_SOCKET);
    close(socket_fd_unix);
    
    
        
    return 0;
}

