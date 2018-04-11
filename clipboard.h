#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#define CLIPBOARD_SOCKET "./CLIPBOARD_SOCKET"
#define INTERNET_SOCKET "INTERNET_SOCKET"
#define NUM_REGIONS 10
#define MESSAGE_MAX_SIZE 100
#define PORT 1337


#define COPY 0
#define PASTE 1

#include <sys/types.h>

typedef struct clipboard_message {
    int region;
    char data[MESSAGE_MAX_SIZE];
    int size;
    int type;
} clipboard_message;


clipboard_message new_message(int region, char *data, int type);
clipboard_message new_copy_message(int region, char *data);
clipboard_message new_paste_message(int region);


int clipboard_connect(char * clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);
#endif