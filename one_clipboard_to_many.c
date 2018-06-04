#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"
#include "utils.h"

/**
 * App that tests data propagation between clipboards
 * */
int main(int argc, char **argv)
{
    char buffer1[25];
    char buffer2[25];
    char buffer3[25];
    
    int client1_id = clipboard_connect("./");
    if (client1_id == -1)
        exit(0);
    
    int client2_id = clipboard_connect("./parent/");
    if (client2_id == -1)
        exit(0);

    int client3_id = clipboard_connect("./child/");
    if (client3_id == -1)
        exit(0);

    sprintf(buffer1, "Mensagem client1");

    int bytes1 = clipboard_copy(client1_id, 3, buffer1, strlen(buffer1) + 1);
    printf("bytes %d\n", bytes1);

    if (clipboard_paste(client2_id, 3, buffer2, strlen(buffer1) + 1) != 0)
    {
        buffer2[strlen(buffer2) + 1] = '\0';
        printf("Parent paste:%s \n", buffer2);
    }

    if(clipboard_paste(client3_id, 3,buffer3,strlen(buffer1)+1) !=0) {
        buffer3[strlen(buffer3)+1] = '\0';
        printf("Parent paste:%s \n", buffer3);
    }
    
    return 0;
}