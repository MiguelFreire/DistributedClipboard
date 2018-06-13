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
#include <errno.h>
#include <pthread.h>
#include "clipboard.h"
#include "clipboard_threads.h"
#include "cblist.h"


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

pthread_mutex_t wait_mutex[NUM_REGIONS];
pthread_cond_t wait_cond[NUM_REGIONS];

pthread_mutex_t region_mutex = PTHREAD_MUTEX_INITIALIZER;

int last_region = -1;

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
        pthread_mutex_destroy(&wait_mutex[i]);
        pthread_cond_destroy(&wait_cond[i]);
    }

    free(store);
    pthread_mutex_destroy(&upper_mutex);
    pthread_cond_destroy(&upper_cond);

    pthread_mutex_destroy(&lower_mutex);
    pthread_cond_destroy(&lower_cond);

    pthread_mutex_destroy(&region_mutex);
    
    unlink(CLIPBOARD_SOCKET);
    
    exit(0);
}

/*
@Name: usage()
@Args: None;
@Desc: Shows a messsage if the program arguments are invalid;
@Return: (void);
*/

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
        pthread_mutex_init(&wait_mutex[j], NULL);
        pthread_cond_init(&wait_cond[j], NULL);
        if(pthread_rwlock_init(&rwlocks[j], NULL) != 0) {
            logs(strerror(errno), L_ERROR);
            exit(-1);
        }
    }

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

