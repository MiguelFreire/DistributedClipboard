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


typedef struct store_object {
    void *data;
    size_t size;
} store_object;

int cbstore(size_t region, void* data, size_t count, store_object *store) {
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

int handleCopy(int client, CBMessage *msg, store_object *store) {
    size_t count;
    void *data_buffer;
    packed_message response;
    size_t bytes;

    //Save data size to count
    count = msg->size;
    //Alloc buffer to save data
    data_buffer = smalloc(count); 
    //Tell the client we are ready to receive data
    response = new_message(Response, msg->method, msg->region,NULL,0,1,1); 
    //Lets clear the message structure to reuse
    cbmessage__free_unpacked(msg,NULL); 
    //Write response to client
    bytes = write(client, response.buf, response.size);
    if(bytes == -1) {
        logs(strerror(errno), L_ERROR);
        free(data_buffer);
        return 0;
    }
    //Read all bytes (count) from client
    bytes = sread(client, data_buffer, count);
    msg = cbmessage__unpack(NULL, count, data_buffer);
    //Store new data in the store, store will be shared var
    cbstore(msg->region, msg->data.data, msg->data.len, store);

    cbmessage__free_unpacked(msg, NULL);
    free(data_buffer);

    return 1;
}

int handlePaste(int client, CBMessage *msg, store_object *store) {
    size_t count;
    void *data_buffer;
    packed_message response;
    packed_message response_with_size;
    uint8_t response_buffer[MESSAGE_MAX_SIZE];
    size_t bytes;

    int region = msg->region;
    printf("REGION: %d\n", region);
    logs("CHECKING", L_INFO);
    if(store[region].data == NULL) {
        logs("Region has no data", L_INFO);
        //there is no data in that region send negative response
        response = new_message(Response, msg->method, msg->region, NULL, 0, 1,0);
        
        cbmessage__free_unpacked(msg, NULL);
        
        bytes = write(client, response.buf, response.size);
        
        free(response.buf);

        return 0;
    }
    logs("NOT OK", L_INFO);
    //Create response with data
    response = new_message(Response, msg->method, region, store[region].data, store[region].size,1,1);
    //Create response with size
    response_with_size = new_message(Response, msg->method, region, NULL, response.size, 0,0);

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

void usage() {
    printf("Usage: \n");
    printf("\t Single Mode: clipboard \n");
    printf("\t Connected Mode: clipboard -c <ip> <port> \n");
    printf("\t \t ip: ipv4 dot format \n");
    printf("\t \t port: integer \n");
}

int main(int argc, char **argv) {
    
    /*Program Vars*/
    int c; //var for opt

    /*Threads*/
    pthread_t thread_regions[NUM_REGIONS];
    pthread_t thread_unix_com_handler;
    pthread_t thread_inet_com_handler;

    /*CLIPBOARD VARIABLES*/
    bool connected_mode;
    char *local_ip;
    int local_port;
    /*UNIX_COM VARIABLES */

    /*INET_COM VARIABLES*/
    char *remote_ip; //remote ip
    int remote_port;  //remote port
    int socket_fd_inet_local; //socket for inet com
    int socket_fd_inet_remote;
    struct sockaddr_in local_addr; //
    struct sockaddr_in remote_addr; // 
    

    /*Handle Arguments / Program Options*/
    while((c=getopt(argc, argv, "c:")) != -1) {
        switch(c) {
            case 'c':
                remote_ip = optarg;
                remote_port = atoi(argv[optind]); //add arguments checker
                connected_mode = true;
                break;
            default:
                logs("No arguments given, launching just in single mode...", L_INFO);
                break;
        }   
    }


    /*new_unix_connection*/
    /*new_inet_connection*/
    srand(time(NULL)); //Use time as seeder for now, maybe change for getpid? 
    local_port = rand()%(64738-1024) + 1024;

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

    
    //Handle Connected Mode

    if(connected_mode) {
        socket_fd_inet_remote = socket(AF_INET,SOCK_STREAM,0);
        if(socket_fd_inet_remote == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(remote_port);
        
        if(inet_aton(remote_ip, &remote_addr.sin_addr) == 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

        if(connect(socket_fd_inet_remote, (const struct sockaddr*) &remote_addr, sizeof(struct sockaddr_in)) == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

    }


    //DONE

    store_object *store;
    store = scalloc(NUM_REGIONS, sizeof(store_object)); //free it in the end!
        


    if(argc == 1) {//single mode
        logs("Starting in single mode.", L_INFO);
        logs("Creating local clipboard", L_INFO);
        //UNIX SOCKET CONNECTION
        //Create address for local clipboard
        struct sockaddr_un clipboard_addr;
        struct sockaddr_un client_addr;

        clipboard_addr.sun_family = AF_UNIX; 
        strcpy(clipboard_addr.sun_path, CLIPBOARD_SOCKET);
        
        //Create socket for local clipboard communication (UNIX DATASTREAM)
        unlink(CLIPBOARD_SOCKET);
        int socket_fd_unix = socket(AF_UNIX, SOCK_STREAM, 0);

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

        //END UNIX SOCKET CONNECTION
        logs("Local server started!", L_INFO); 

        
        CBMessage *msg;
        
        uint8_t size_buffer[MESSAGE_MAX_SIZE];
        int bytes = 0;
        int client;
        unsigned int addr_size = sizeof(client_addr);
        int status;


        while(1) {
            logs("Waiting for clients...", L_INFO);
            int client = accept(socket_fd_unix, (struct sockaddr *) &client_addr, &addr_size);
            
            if(client == -1) {
                logs(strerror(errno), L_ERROR);
                exit(-1);
            }
            
            if(fork() == 0) {
                logs("Client connected!", L_INFO);
                while(1) {
                    bytes = read(client, size_buffer, MESSAGE_MAX_SIZE);
                    if(bytes == 0) {
                        logs("Client disconnected", L_INFO);
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
                            status = handleCopy(client, msg, store);
                            status ? logs("Copy method handled successfuly", L_INFO) : logs("Error handlying copy method", L_ERROR);
                            break;
                        case Paste:
                            logs("Handling paste method...", L_INFO);
                            status = handlePaste(client, msg, store);
                            status ? logs("Paste method handled successfuly", L_INFO) : logs("Error handlying paste method", L_ERROR);
                            break;
                        default:
                            break;
                    }
                }
                
                //First we read the request message with the information about size,region and method
            }

        }
        
        unlink(CLIPBOARD_SOCKET);
        close(socket_fd_unix);
        
        exit(0);
    } if(argc > 3 && argv[1][0] == '-' && argv[1][1] == 'c') {
        struct sockaddr_in server_addr;
        int socket_fd;
        int port;
        //Check for valid IP
        if(inet_aton(argv[2], &server_addr) == 0) {
            logs("Invalid IP Address", L_ERROR);
            usage();
            exit(-1);
        }


        socket_fd = socket(AF_INET, SOCK_STREAM,0);
        
        server_addr.sin_family = AF_INET;
	    server_addr.sin_port= htons(port);

    }
        
    return 0;
}

