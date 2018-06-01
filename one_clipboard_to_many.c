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
     char buffer1[25];
     char buffer2[25];
     char buffer3[25];
     int client1_id = clipboard_connect("./");
     int client2_id = clipboard_connect("./parent/");
     int client3_id = clipboard_connect("./child/");


             sprintf(buffer1, "Mensagem client1");
             
             int bytes1 = clipboard_copy(client1_id, 3, buffer1, strlen(buffer1)+1);
             printf("bytes %d\n",bytes1);
             if (clipboard_paste(client2_id, 3, buffer2, strlen(buffer1) + 1) != 0) {
                 buffer2[strlen(buffer2)+1] = '\0';
                 printf("Parent paste:%s \n", buffer2);
             }
            if(clipboard_paste(client3_id, 3,buffer3,strlen(buffer1)+1) !=0) {
                 buffer3[strlen(buffer3)+1] = '\0';
                 printf("Parent paste:%s \n", buffer3);

             }
    
    return 0;
}