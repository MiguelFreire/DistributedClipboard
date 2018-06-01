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
    
    for (i=0; i<10; i++)
    {
        if(fork()==0)
        {
             char buffer[25];
             int client_id = clipboard_connect("./");
             sprintf(buffer, "Mensagem client %d",i);
             int bytes = clipboard_copy(client_id, 3, buffer, strlen(buffer)+1);
             printf("bytes %d",bytes);
             exit(0);
        }
    }

    return 0;
}