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


int main(int argc, char **argv)
{
    int i=0;
    char buffer1[25];
    int client1_id = clipboard_connect("./");    
    if (client1_id == -1)
        {
             logs("Error connecting to local clipboard", L_ERROR);
             exit(-1);
        }
    
    for (i=0; i<10; i++)
    {
        if(fork()==0)
        {
             char buffer[25];
             int client_id = clipboard_connect("./");
             if (client_id == -1)
            {
                 logs("Error connecting to local clipboard", L_ERROR);
                 exit(-1);
            }
             
             if (clipboard_wait(client_id, 3, buffer, 25) != 0) {
                buffer[strlen(buffer)+1] ='\0';
                printf("Paste:%s \n", buffer);
            }
             exit(0);
        }
    }
    sleep(5);
     sprintf(buffer1, "Mensagem cliente %d",i+1);
     int bytes=clipboard_copy(client1_id,3,buffer1, strlen(buffer1)+1);
     printf("bytes %d\n",bytes);

    return 0;
}