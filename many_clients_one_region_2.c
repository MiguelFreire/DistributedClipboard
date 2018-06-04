#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"
#include "utils.h"

/**
 * App that tests many clients copying and pasting from the same region
 * */
int main(int argc, char **argv)
{
    int i=0;
    int N = 10;

    if (argc > 1) {
        N = atoi(argv[1]);
    }

    for (i=0; i < N; i++)
    {
        if(fork()==0) {
            char buffer[25];
            char buffer1[25];
            
            int client_id = clipboard_connect("./");
            if(client_id == -1) exit(0);

            sprintf(buffer, "Mensagem client %d",i);
            
            int bytes = clipboard_copy(client_id, 3, buffer, strlen(buffer)+1);
            
            printf("bytes %d\n",bytes);
            if (clipboard_paste(client_id, 3, buffer1, strlen(buffer) + 1) != 0) {
                buffer1[strlen(buffer)+1] = '\0';
                printf("Paste:%s \n", buffer1);
            }
            clipboard_close(client_id);
            exit(0);
        }
    }

    return 0;
}