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

    int clipboard_id = clipboard_connect("./");

    if(clipboard_id == -1) {
        logs("Error connecting to local clipboard", L_ERROR);
        exit(-1);
    }
    
    char buffer[MESSAGE_MAX_SIZE];
    char buffer2[MESSAGE_MAX_SIZE];
    char buffer3[MESSAGE_MAX_SIZE];

    strcpy(buffer, "OLA MUNDO!");
    strcpy(buffer2, "ADEUS");
    strcpy(buffer3, "CRL!");

    clipboard_copy(clipboard_id, 2, buffer, strlen(buffer));
    clipboard_copy(clipboard_id, 3, buffer2, strlen(buffer2));
    //clipboard_paste(clipboard_id, 0, buffer2, strlen(buffer)+1);
    printf("Paste Client: %s \n", buffer2);
    

    return 0;
}