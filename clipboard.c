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

        
        
        char buffer[MESSAGE_MAX_SIZE];
        clipboard_message request;
        clipboard_message response;

        int bytes = 0;
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
                char bf[100];
                
                sprintf(bf, "Client %d connected", client);
                logs((char *)bf, L_INFO);
                printf("Clipboard message size: %d\n", sizeof(clipboard_message));
                while (read(client, &request, sizeof(clipboard_message)) > 0)
                {
                    printf("Entrei!\n");
                    printf("Metodo: %d\n", request.method);
                    switch (request.method)
                    {
                    case Copy:
                        printf("Request Copy received! - %s \n", request.data);
                        
                        if(store[request.region] != NULL) 
                            free(store[request.region]);

                        store[request.region] = smalloc(request.size);
                        memcpy(store[request.region], request.data, request.size);
                        printf("LALALLAA\n");
                        response.status = true;
                        break;
                    case Paste:  
                        printf("Request Paste received! -  Region %d \n", request.region);
                        response.status = true;
                        memcpy(response.data, store[request.region], request.size);
                        break;
                    default:
                        logs("Invalid Request method", L_ERROR);
                    }
                    printf("Teste \n");
                    write(client, &response, sizeof(response));
                }
                
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

