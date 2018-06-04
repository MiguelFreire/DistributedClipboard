#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "clipboard.h"
#include "utils.h"

/**
 * App that tests many clients copying to diferent regions in the same clipboard
 * */
int main(int argc, char **argv)
{
    int i=0;
    int N = 10;

    if (argc > 1)
    {
        N = atoi(argv[1]);
    }

    for (i=0; i < N; i++)
    {
        if(fork()==0)
        {
            char buffer[25];
            
            int client_id = clipboard_connect("./");
            if (client_id == -1) exit(0);

            srand(time(NULL));
            int region = rand() % 10;

            sprintf(buffer, "Mensagem client %d",i);
            
            int bytes = clipboard_copy(client_id, region, buffer, strlen(buffer)+1);
            
            printf("buffer %s\n",buffer);
            printf("bytes %d\n",bytes);
            clipboard_close(client_id);
            exit(0);
        }
    }

    return 0;
}