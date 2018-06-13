#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clipboard.h"
#include "utils.h"

/**
 * App that tests many clients copying to the same region
 * */
int main(int argc, char **argv)
{

     int client_id = clipboard_connect("./");
     char linha[100];
     int regiao = 0;
     char comando;

    while(1) {
        fgets(linha, 100, stdin);
        sscanf(linha, "%d", &regiao);
        fgets(linha, 100, stdin);
        sscanf(linha, "%c", &comando);

        if(comando == 'c') {
            fgets(linha, 100, stdin);
            clipboard_copy(client_id, regiao, linha, 100);
        }
        if(comando == 'p') {
            bzero(linha,100);
            clipboard_paste(client_id, regiao, linha, 100);
            printf("\n Linha: %s \n", linha);
        }
        if (comando == 'w')
        {
            clipboard_wait(client_id, regiao, linha, 100);
            printf("Linha: %s \n", linha);
        }
    }


    return 0;
}