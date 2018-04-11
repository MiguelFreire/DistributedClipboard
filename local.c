#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
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

    char store[10][MESSAGE_MAX_SIZE]; //create store 

    if(argc == 1) {//single mode
        logs("Starting in single mode.", L_INFO);
        logs("Creating local clipboard", L_INFO);

        //Create address for local clipboard
        struct sockaddr_un local_addr;

        local_addr.sun_family = AF_UNIX;
        strcpy(local_addr.sun_path, CLIPBOARD_SOCKET);
        
        //Create socket for local clipboard communication (UNIX DATASTREAM)
        unlink(CLIPBOARD_SOCKET);
        int local_socket = socket(AF_UNIX, SOCK_DGRAM, 0);

        if(local_socket == -1) {
            logs(strerror(errno), L_ERROR);
            exit(1);
        } 

        //Bind socket to address

        if(bind(local_socket, (struct sockaddr *) &local_addr, sizeof(local_addr)) == -1) {
            logs(strerror(errno), L_ERROR);
            exit(1);
        }

        logs("Local server started!", L_INFO);   

        exit(0);
    } if(argc > 3 && argv[1][0] == '-' && argv[1][1] == 'c') {
        struct in_addr server_addr;

        if(inet_aton(argv[2], &server_addr) == 0) {
            printf("Invalid IP Address\n");
            usage();
            exit(-1);
        }

        int port = atoi(argv[3]);


    }
        
    return 0;
}

