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

    if (clipboard_id == -1)
    {
        logs("Error connecting to local clipboard", L_ERROR);
        exit(-1);
    }

    char *teste_f, *teste_2f;

    teste_f = smalloc(2048);
    teste_2f = smalloc(2048);

    for (int i = 0; i < 2048; i++)
    {
        teste_f[i] = 'c';
    }
    teste_f[2048] = '\0';

    clipboard_copy(clipboard_id, 3, teste_f, 2048);
    getchar();
    clipboard_paste(clipboard_id, 3, teste_2f, 2048);
    printf("Paste client:%s \n", teste_2f);

    return 0;
}