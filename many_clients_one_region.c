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
     int client1_id = clipboard_connect("./");
     int client2_id = clipboard_connect("./");
     int client3_id = clipboard_connect("./");
     int client4_id = clipboard_connect("./");
     int client5_id = clipboard_connect("./");
     int client6_id = clipboard_connect("./");
     int client7_id = clipboard_connect("./");
     int client8_id = clipboard_connect("./");
     int client9_id = clipboard_connect("./");
     int client10_id = clipboard_connect("./");

     char teste1[]="Mensagem cliente 1";
     char teste2[]="Mensagem cliente 2";
     char teste3[]="Mensagem cliente 3";
     char teste4[]="Mensagem cliente 4";
     char teste5[]="Mensagem cliente 5";
     char teste6[]="Mensagem cliente 6";
     char teste7[]="Mensagem cliente 7";
     char teste8[]="Mensagem cliente 8";
     char teste9[]="Mensagem cliente 9";
     char teste10[]="Mensagem cliente 10";
     char teste11[strlen(teste10)]="";
    
    clipboard_copy(client1_id, 3, teste1, strlen(teste1)+1);
    clipboard_copy(client2_id, 3, teste2, strlen(teste2)+1);
    clipboard_copy(client3_id, 3, teste3, strlen(teste3)+1);
    clipboard_copy(client4_id, 3, teste4, strlen(teste4)+1);
    clipboard_copy(client5_id, 3, teste5, strlen(teste5)+1);
    clipboard_copy(client6_id, 3, teste6, strlen(teste6)+1);
    clipboard_copy(client7_id, 3, teste7, strlen(teste7)+1);
    clipboard_copy(client8_id, 3, teste8, strlen(teste8)+1);
    clipboard_copy(client9_id, 3, teste9, strlen(teste9)+1);
    clipboard_copy(client10_id, 3, teste10, strlen(teste10)+1);

    clipboard_paste(clipboard_id, 3, teste11, strlen(teste11)+1);
    printf("Paste client:%s \n", teste11);
    
    free(teste1);
    free(teste2);
    free(teste3);
    free(teste4);
    free(teste5);
    free(teste6);
    free(teste7);
    free(teste8);
    free(teste9);
    free(teste10);
    free(teste11);
    return 0;



}