#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include "clipboard.h"
#include "cbmessage.pb-c.h"
#include "utils.h"
#include "cblist.h"

typedef struct store_object {
    void *data;
    size_t size;
} store_object;

typedef enum client_t {
    App,
    Clipboard,
    Parent
} client_t;

typedef struct thread_arg {
    int client;
    client_t type;
} thread_arg;

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

struct sockaddr_in local_addr; 
struct sockaddr_in remote_addr; 
struct sockaddr_un clipboard_addr;
struct sockaddr_un client_addr;

connected_list *cblist;
connected_list *applist;
pthread_rwlock_t rwlocks[NUM_REGIONS];

pthread_mutex_t upper_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t upper_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t lower_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lower_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t wait_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_cond = PTHREAD_COND_INITIALIZER;

int last_region = -1;
int uport = 0;

/*
@Name: terminate_handler()
@Arg: (int) signum - signal id
@Desc: Handles ctrl+c
@Return: void
*/

void terminate_handler(int signum) {
    
    if(connected_mode) {
        pthread_cancel(thread_upper_com_handler);
        pthread_cancel(thread_child);
    }
    close(socket_fd_inet_local);
    close(socket_fd_inet_remote);  
    close(socket_fd_inet_child);

    if(cblist->size > 0) {
        free_list(cblist);
    }

    for(int i = 0; i < NUM_REGIONS; i++) {
        if(store[i].size > 0) free(store[i].data);
        pthread_rwlock_destroy(&rwlocks[i]); 
    }
    // printf("OLA\n");
    unlink(CLIPBOARD_SOCKET);
    
    exit(0);
}

/*
@Name: new_sync_message()
@Arg: None;
@Desc: Creates a message to sync with another clipboard;
@Return: struct packed_message;
*/

packed_message new_sync_message()
{
    CBMessage msg = CBMESSAGE__INIT;

    size_t packed_size;
    void* buffer;
    packed_message package = {NULL, 0};
    store_object *replica;
    replica = scalloc(NUM_REGIONS, sizeof(store_object));

    msg.type = Request;
    msg.method = Sync;

    msg.data = scalloc(NUM_REGIONS, sizeof(ProtobufCBinaryData));
                                            
    for(int i = 0; i < NUM_REGIONS; i++) {
        msg.n_data++;
        if(store[i].size > 0) {
            pthread_rwlock_rdlock(&rwlocks[i]);
            replica[i].size = store[i].size;
            replica[i].data = smalloc(sizeof(replica[i].size));
            memcpy(replica[i].data, store[i].data, replica[i].size);
            pthread_rwlock_unlock(&rwlocks[i]);
            
            msg.data[i].data = replica[i].data;
            msg.data[i].len = replica[i].size;
        }
    }

    packed_size = cbmessage__get_packed_size(&msg);
   
    buffer = smalloc(packed_size);
    cbmessage__pack(&msg, buffer);

    package.buf = buffer;
    package.size = packed_size;
    
    for(int i = 0; i < NUM_REGIONS; i++) {
        if(replica[i].data != NULL) free(replica[i].data);
    }
    free(replica);
    free(msg.data);

    return package;
}

/*
@Name: cbstore()
@Args: (size_t) region - region number;
       (void *) data - pointer to data buffer;
       (size_t) count - number of bytes to be copied;
@Desc: Saves data in store/regions;
@Return: (int) 1 if the data has been saved correctly;
              -1 if something went wrong;
*/

int cbstore(size_t region, void* data, size_t count) {
   if(!validate_region(region) || data == NULL || store == NULL)
        return -1;
    //We dont need locks here because it's done in init time
    if(store[region].data != NULL) {
        free(store[region].data);
        store[region].size = 0;
    }

    store[region].data = smalloc(count);
    memcpy(store[region].data, data, count);
    store[region].size = count;

    return 1;
}

/*
@Name: clipboard_sync()
@Args: (int) clipboard_id - id of the clipboard that sends a request do sync;
@Desc: Performs a syncronization request to the clipboard identified by clipboard_id;
@Return: (int) size of all regions;
*/

int clipboard_sync(int clipboard_id) {
    CBMessage *msg;
    int bytes = 0;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t size;
    void *buffer;
    packed_message response;
    packed_message request = new_message(Request, Sync, 0, NULL,0,0,0,0,0);


    bytes = write(clipboard_id, request.buf, request.size);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }

    bytes = read(clipboard_id, response_buffer, MESSAGE_MAX_SIZE);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        return 0;
    }

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
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }


    bytes = sread(clipboard_id, buffer, size);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(request.buf);
        free(response.buf);
        return 0;
    }

    msg = cbmessage__unpack(NULL, size, buffer);

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
/*
@Name: handleSync()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles syncronization request;
@Return: (int) 1 if the sync request was handled right;
               0 if something went wrong with the comunication;
*/
 
 int handleSync(int client, CBMessage *msg) {
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    CBMessage *user_msg;
    

    //Create response with data
    response = new_sync_message();
    //Create response with size
    response_with_size = new_message(Response, msg->method, 0, NULL, response.size, 0,0,0,0);

    bytes = write(client, response_with_size.buf, response_with_size.size);
    
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    user_msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(user_msg->has_status && user_msg->status) {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if(bytes == -1 || !bytes) {
            if(bytes == -1) logs(strerror(errno), L_ERROR);
            free(response.buf);
            free(response_with_size.buf);
            return 0;
        }
    }

    cbmessage__free_unpacked(user_msg, NULL);

    free(response.buf);

    return 1;
}
/*
@Name: handleCopy()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles copy request;
@Return: (int) 1 if the copy request was handled right;
               0 if something went wrong with the comunication;
*/

int handleCopy(int client, CBMessage *msg) {
    size_t count;
    void *data_buffer;
    packed_message response;
    size_t bytes;
    CBMessage *user_msg;

    //Save data size to count
    count = msg->size;
    //Alloc buffer to save data
    data_buffer = smalloc(count); 
    //Tell the client we are ready to receive data
    response = new_message(Response, msg->method, msg->region,NULL,0,1,1,0,0); 
    //Lets clear the message structure to reuse

    //Write response to client
     bytes = write(client, response.buf, response.size);
   
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(data_buffer);
        return 0;
    }

    //Read all bytes (count) from client
    bytes = sread(client, data_buffer, count);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(data_buffer);
        return 0;
    }
   
    user_msg = cbmessage__unpack(NULL, count, data_buffer);
    //Store new data in the store, store will be shared var

    pthread_rwlock_wrlock(&rwlocks[user_msg->region]);
    cbstore(user_msg->region, user_msg->data->data, user_msg->data->len);
    pthread_rwlock_unlock(&rwlocks[user_msg->region]);


    cbmessage__free_unpacked(user_msg, NULL);
    free(response.buf);
    free(data_buffer);

    return 1;
}
/*
@Name: handlePaste()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles paste request;
@Return: (int) 1 if the paste request was handled right;
               0 if something went wrong with the comunication;
*/

int handlePaste(int client, CBMessage *msg) {
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    CBMessage *user_msg;

    int region = msg->region;
      logs("CHECKING", L_INFO);

    /*Read LOCK!*/
    pthread_rwlock_rdlock(&rwlocks[region]);
    if(store[region].data == NULL) {
        logs("Region has no data", L_INFO);
        //there is no data in that region send negative response
        response = new_message(Response, msg->method, msg->region, NULL, 0, 1,0,0,0);
         
        pthread_rwlock_unlock(&rwlocks[region]);
        bytes = write(client, response.buf, response.size);
        if(bytes == -1 || !bytes) {
            if(bytes == -1) logs(strerror(errno), L_ERROR);
            free(response.buf);
            return 0;
        }
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
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    user_msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if(user_msg->has_status && user_msg->status) {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if(bytes == -1 || !bytes) {
            if(bytes == -1) logs(strerror(errno), L_ERROR);
            free(response.buf);
            free(response_with_size.buf);
            return 0;
    }
    }

    cbmessage__free_unpacked(user_msg, NULL);

    free(response.buf);

    return 1;
}
/*
@Name: handleWait()
@Args: (int) client - id of the client socket;
       (CBMessage*) msg - pointer to the message recieved;
@Desc: Handles wait request;
@Return: (int) 1 if the wait request was handled right;
               0 if something went wrong with the comunication;
*/
int handleWait(int client, CBMessage *msg)
{
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;
    CBMessage *user_msg;

    int region = msg->region;
 
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
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    //Get ready response from client
    bytes = read(client, response_buffer, MESSAGE_MAX_SIZE);
    if(bytes == -1 || !bytes) {
        if(bytes == -1) logs(strerror(errno), L_ERROR);
        free(response.buf);
        free(response_with_size.buf);
        return 0;
    }
    user_msg = cbmessage__unpack(NULL, bytes, response_buffer);

    if (user_msg->has_status && user_msg->status)
    {
        //Client said it's ok! Let's send the data!
        bytes = write(client, response.buf, response.size);
        if(bytes == -1 || !bytes) {
            if(bytes == -1) logs(strerror(errno), L_ERROR);
            free(response.buf);
            free(response_with_size.buf);
            return 0;
        }
    }

    cbmessage__free_unpacked(user_msg, NULL);

    free(response.buf);

    return 1;
}
/*
@Name: usage()
@Args: None;
@Desc: Shows a messsage if the program arguments are invalid;
@Return: (void);
*/

void usage() {
    printf("Usage: \n");
    printf("\t Single Mode: clipboard \n");
    printf("\t Connected Mode: clipboard -c <ip> <port> \n");
    printf("\t \t ip: ipv4 dot format \n");
    printf("\t \t port: integer \n");
}
/*
@Name: thread_lower_com()
@Args: (void *) arg;
@Desc: Thread that handles all lower communication copy from parent to all children;
@Return: (void *);
*/

void *thread_lower_com(void *arg) {
    int l_region;
    int bytes;
    cb_client *cb;
    logs("Lower Com Thread launched", L_INFO);
    while(1) {
        pthread_mutex_lock(&lower_mutex);
        pthread_cond_wait(&lower_cond, &lower_mutex);
        l_region = last_region;
        pthread_rwlock_rdlock(&rwlocks[l_region]);
        pthread_mutex_lock(&(cblist->mutex));
        cb = cblist->cb;
        
        while(cb != NULL) {
            bytes = clipboard_lower_copy(cb->socket_teste, l_region, store[l_region].data, store[l_region].size);
            cb = cb->next;
        }
        pthread_mutex_unlock(&(cblist->mutex));
        pthread_rwlock_unlock(&rwlocks[l_region]);
        pthread_mutex_unlock(&lower_mutex);
    }
}
/*
@Name: thread_upper_com()
@Args: (void *) arg;
@Desc: Thread that handles all communication from children to the parent;
@Return: (void *);
*/

void *thread_upper_com(void *arg) {
    int l_region;
    int bytes;
    logs("Thread Upper com handler started!",L_INFO);
    while(1) {
        pthread_mutex_lock(&upper_mutex);
        pthread_cond_wait(&upper_cond, &upper_mutex);
        l_region = last_region;
        pthread_rwlock_rdlock(&rwlocks[l_region]);
        
        bytes = clipboard_copy(socket_fd_inet_remote, l_region, store[l_region].data, store[l_region].size);
        
        if(bytes == -1 || !bytes) {
            connected_mode = 0;
            pthread_rwlock_unlock(&rwlocks[l_region]);
            pthread_mutex_unlock(&upper_mutex);
            pthread_exit(NULL);
        } 

        pthread_rwlock_unlock(&rwlocks[l_region]);
        pthread_mutex_unlock(&upper_mutex);
    }
}
/*
@Name: requestHandler()
@Args: (CBMessage *) msg - pointer to the message recieved;
       (int) client - id of the client socket;
@Desc: Handles the request by type;
@Return: (void);
*/

void requestHandler(CBMessage *msg, int client)
{
    int status = 0;
    switch (msg->method)
    {
    case Copy:
        logs("Handling copy method...", L_INFO);
        status = handleCopy(client, msg);
        status ? logs("Copy method handled successfuly", L_INFO) : logs("Error handlying copy method", L_ERROR);
        
        pthread_mutex_lock(&wait_mutex);
        last_region = msg->region;
        pthread_cond_broadcast(&wait_cond);
        pthread_mutex_unlock(&wait_mutex);

        if (connected_mode && status && !msg->lower_copy)
        {
            logs("Sending copy call to parent", L_INFO);
            pthread_mutex_lock(&upper_mutex);
            pthread_cond_signal(&upper_cond);
            pthread_mutex_unlock(&upper_mutex);
        }
        else if ((!connected_mode || msg->lower_copy) && (cblist->size > 0) && status)
        {
            logs("Sending copy call to children", L_INFO);
            pthread_mutex_lock(&lower_mutex);
            pthread_cond_signal(&lower_cond);
            pthread_mutex_unlock(&lower_mutex);
        }
        break;
    case Paste:
        logs("Handling paste method...", L_INFO);
        status = handlePaste(client, msg);
        status ? logs("Paste method handled successfuly", L_INFO) : logs("Error handling paste method", L_ERROR);
        break;
    case Sync:
        logs("Handling sync method...", L_INFO);
        status = handleSync(client, msg);
        status ? logs("Sync method handled successfuly", L_INFO) : logs("Error handling sync method", L_ERROR);
        break;
    case Wait:
        logs("Handling wait method...", L_INFO);
        status = handleWait(client, msg);
        status ? logs("Wait methond handled successfuly", L_INFO) : logs("Error handling wait method", L_ERROR);
    default:
        break;
    }


}
/*
@Name: thread_client()
@Args: (void *) arg;
@Desc: Thread that handles all types of network communication:
    -> app-clipboard;
    -> parent-children;
    -> clipboard-clipboard;
@Return: (void *);
*/

void *thread_client(void *arg) {
    thread_arg *args = (thread_arg *)arg;
    int client = args->client;
    client_t client_type = args->type;
    CBMessage *msg;

    uint8_t size_buffer[MESSAGE_MAX_SIZE];
    int bytes = 0;

    logs("Clipboard connected!", L_INFO);
    while (1)
    {
        bzero(size_buffer, MESSAGE_MAX_SIZE);
        bytes = read(client, size_buffer, MESSAGE_MAX_SIZE);
        if (bytes == 0)
        {
            switch(client_type) {
                case Clipboard:
                    remove_clipboard_by_thread_id(cblist, pthread_self());
                    logs("CB removed from cb list", L_INFO);
                    break;
                case App:
                    remove_clipboard_by_thread_id(applist, pthread_self());
                    logs("App removed from app list", L_INFO);
                    break;
                case Parent:
                    connected_mode = 0;
                    logs("Parent died!", L_INFO);
                    break;
                default:
                    break;
            }
            break;
        }
        if (bytes == -1)
        {
            logs(strerror(errno), L_ERROR);
        }

        //Unpacks from proto to c format
        msg = cbmessage__unpack(NULL, bytes, size_buffer);

        requestHandler(msg, client);
        cbmessage__free_unpacked(msg, NULL);
    }
    logs("Client disconnected!", L_INFO);
    logs("Thread terminating!", L_INFO);

    return 0;
}
/*
@Name: thread_inet_handler()
@Args: (void *) arg;
@Desc: Thread that waits and accepts internet clients (clipboards);
@Return: (void *);
*/
void *thread_inet_handler(void * arg) {
        unsigned int addr_size = sizeof(client_addr);
        int parent;
        int child;        
        cb_client *rcb;

        while(1) {
            logs("Waiting for remote clients...", L_INFO);
            rcb = NULL;
            
            parent = accept(socket_fd_inet_local, (struct sockaddr *) &client_addr, &addr_size);
            if(parent == -1) {
                printf("AQUI!\n");
                logs(strerror(errno), L_ERROR);
                pthread_exit(NULL);
            }
            child = accept(socket_fd_inet_local, (struct sockaddr *)&client_addr, &addr_size);
            if(child == -1) {
                printf("AQUI!2\n");
                logs(strerror(errno), L_ERROR);
                pthread_exit(NULL);
            }
            
            rcb = new_clipboard(parent, child);
            add_clipboard(cblist, rcb);
            
            if(parent == -1) {
                printf("AQUI!3\n");
                logs(strerror(errno), L_ERROR);
                pthread_exit(NULL);
            }

            thread_arg args;
            args.client = rcb->socket_fd;
            args.type = Clipboard;

            if(pthread_create(&rcb->thread_id, NULL, thread_client, &args) != 0) {
                printf("AQUI4!\n");
                logs(strerror(errno), L_ERROR);
                pthread_exit(NULL);
            }
            
        }

    return 0;
}
/*
@Name: configure_unix_com()
@Args: None;
@Desc: Configues sockets for unix domains communication;
@Return: (void);
*/

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

/*
@Name: configure_inet_local_com()
@Args: None;
@Desc: Configues sockets for the clipboard to accept remote connections;
@Return: (void);
*/

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
/*
@Name: configure_inet_remote_com()
@Args: None;
@Desc: Configues sockets for internet domains communication;
@Return: (void);
*/

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
    struct sigaction intHandler;

    intHandler.sa_handler = &terminate_handler;
    sigaction(SIGINT, &intHandler, NULL);
    /*Program Vars*/
    int c; //var for opt
    logs("Store created", L_INFO);
    //TODO: create new store function
    store = scalloc(NUM_REGIONS, sizeof(store_object)); 
    cblist = new_list();
    applist = new_list();

    thread_arg child_args;
    thread_arg app_args;

    logs("Creating read-write locks...", L_INFO);
    for(int j = 0; j < NUM_REGIONS; j++) {
        if(pthread_rwlock_init(&rwlocks[j], NULL) != 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }
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
        
        //Create communication thread for parent
        if(pthread_create(&thread_upper_com_handler, NULL, thread_upper_com, NULL) != 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }
        //Create communication thread for children
        child_args.client = socket_fd_inet_child;
        child_args.type = Parent;

        if(pthread_create(&thread_child, NULL, thread_client, &child_args) != 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }
    }

    if(pthread_create(&thread_lower_com_handler, NULL, thread_lower_com, NULL) != 0) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }
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
    cb_client *app;
    
    
    if(pthread_create(&thread_inet_com_handler, NULL, thread_inet_handler, NULL) != 0) {
        logs(strerror(errno), L_ERROR);
        exit(-1);
    }

    while(1) {
        logs("Waiting for local clients...", L_INFO);
        client = accept(socket_fd_unix, (struct sockaddr *) &client_addr, &addr_size);
        
        if(client == -1) {
            logs(strerror(errno), L_ERROR);
        }
        app = new_clipboard(client,0);
        add_clipboard(applist, app);
        
        
        app_args.client = app->socket_fd;
        app_args.type = App;

        if(pthread_create(&app->thread_id, NULL, thread_client, &app_args) != 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

    }
    //TODO: free store and free cblist
    unlink(CLIPBOARD_SOCKET);
    close(socket_fd_unix);
    
    
        
    return 0;
}

