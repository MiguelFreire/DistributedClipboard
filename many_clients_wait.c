#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"
#include "utils.h"

/**
 * App that tests many clients waiting and one copying to a region
 * */
int main(int argc, char **argv)
{
    int i=0;
    int N = 10;

    if(argc > 1) {
        N = atoi(argv[1]);
    }

    char buffer1[25];
    int client1_id = clipboard_connect("./");    
    if (client1_id == -1)
        {
             logs("Error connecting to local clipboard", L_ERROR);
             exit(-1);
        }
    
    for (i=0; i < N; i++)
    {
        if(fork()==0)
        {
            char buffer[25];
            int client_idb = clipboard_connect("./");
            if (client_idb == -1) {
                logs("Error connecting to local clipboard", L_ERROR);
                exit(-1);
            }
             
            if (clipboard_wait(client_idb, 3, buffer, 25) != 0) {
                buffer[strlen(buffer)+1] ='\0';
                printf("Paste:%s \n", buffer);
            }
            clipboard_close(client_idb);
            exit(0);
        }
    }
    sleep(5);
    sprintf(buffer1, "Mensagem cliente %d",i+1);
    int bytes=clipboard_copy(client1_id,3,buffer1, strlen(buffer1)+1);
    printf("bytes %d\n",bytes);

    clipboard_close(client1_id);

    return 0;
}