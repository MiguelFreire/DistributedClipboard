#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#define CLIPBOARD_SOCKET "./CLIPBOARD_SOCKET"
#define CLIENT_SOCKET "./CLIENT_SOCKET"
#define INTERNET_SOCKET "INTERNET_SOCKET"
#define NUM_REGIONS 10
#define MESSAGE_MAX_SIZE 1024
#define PORT 1337
#define BACKLOG 5
#define TIMEOUT 5

#define COPY 0
#define PASTE 1

#include <sys/types.h>
#include <stdbool.h>
#include "cbmessage.pb-c.h"

#define Request CBMESSAGE__TYPE__Request
#define Response CBMESSAGE__TYPE__Response
#define Copy CBMESSAGE__METHOD__Copy
#define Paste CBMESSAGE__METHOD__Paste
#define Sync CBMESSAGE__METHOD__Sync

typedef CBMessage__Type message_type;
typedef CBMessage__Method message_method;

typedef struct packed_message {
    void *buf;
    int size;
} packed_message;

packed_message new_message(message_type type, message_method method, int region, void *data, size_t count, bool has_status, bool status, bool has_lower_copy, bool lower_copy);



int clipboard_connect(char *clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_lower_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

bool validate_region(int region);
#endif