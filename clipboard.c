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

#include "clipboard.h"
#include "cbmessage.pb-c.h"
#include "utils.h"


void usage() {
    printf("Usage: \n");
    printf("\t Single Mode: clipboard \n");
    printf("\t Connected Mode: clipboard -c <ip> <port> \n");
    printf("\t \t ip: ipv4 dot format \n");
    printf("\t \t port: integer \n");
}

int main(int argc, char **argv) {

    void * store[NUM_REGIONS]; //create store 

    if(argc == 1) {//single mode
        logs("Starting in single mode.", L_INFO);
        logs("Creating local clipboard", L_INFO);

        //Create address for local clipboard
        struct sockaddr_un clipboard_addr;
        struct sockaddr_un client_addr;

        clipboard_addr.sun_family = AF_UNIX; 
        strcpy(clipboard_addr.sun_path, CLIPBOARD_SOCKET);
        
        //Create socket for local clipboard communication (UNIX DATASTREAM)
        unlink(CLIPBOARD_SOCKET);
        int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

        if(socket_fd == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        } 

        //Bind socket to address

        if(bind(socket_fd, (struct sockaddr *) &clipboard_addr, sizeof(clipboard_addr)) == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }

        if(listen(socket_fd,1) == -1) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }


        logs("Local server started!", L_INFO); 

        
        

        CBMessage *msg;
        packed_message response;
        uint8_t size_buffer[MESSAGE_MAX_SIZE];
        void *data_buffer = NULL; //byte stream
        int bytes = 0;
        size_t count = 0;
        int client;
        int addr_size = sizeof(client_addr);


        while(1) {
            logs("Waiting for clients...", L_INFO);
            int client = accept(socket_fd, (struct sockaddr *) &client_addr, &addr_size);
            if (client == -1)
            {
                logs(strerror(errno), L_ERROR);
                exit(-1);
            }
            if(fork() == 0) {
                logs("Client connected!", L_INFO);
                //First we read the request message with the information about size,region and method
                bytes = read(client, size_buffer, MESSAGE_MAX_SIZE);
                if(bytes == -1) {
                    logs(strerror(errno), L_ERROR);
                }
                printf("Received %d\n", bytes);
                //Unpacks from proto to c format
                msg = cbmessage__unpack(NULL, bytes, size_buffer);
                
                //Get data size
                count = msg->size;
                printf("Region: %d, Method: %d, Size:%d \n", msg->region, msg->method, msg->size);
                //Alloc data receive buffer
                data_buffer = smalloc(count);
                response = new_response(msg->method,
                                        msg->region,
                                        NULL,
                                        0,
                                        true);
                cbmessage__free_unpacked(msg, NULL);

                bytes = write(client, response.buf, response.size);
                if (bytes == -1)
                {
                    logs(strerror(errno), L_ERROR);
                }
                bytes = sread(client, data_buffer, count);
                if (bytes == -1)
                {
                    logs(strerror(errno), L_ERROR);
                }

                logs("Received2!", L_INFO);
                msg = cbmessage__unpack(NULL, count, data_buffer);
                printf("Received: %s\n", msg->data.data);
            

                cbmessage__free_unpacked(msg, NULL);
                free(data_buffer);
            }

        }
        
        unlink(CLIPBOARD_SOCKET);
        close(socket_fd);
        
        exit(0);
    } if(argc > 3 && argv[1][0] == '-' && argv[1][1] == 'c') {
        struct in_addr server_addr;

        if(inet_aton(argv[2], &server_addr) == 0) {
            logs("Invalid IP Address", L_ERROR);
            usage();
            exit(-1);
        }

        int port = atoi(argv[3]);


    }
        
    return 0;
}

