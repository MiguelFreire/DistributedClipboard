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

typedef enum message_type {
    Request,
    Response
} message_type;

typedef enum message_method
{
    Copy,
    Paste
} message_method;

typedef struct clipboard_message {
    message_type type; //required
    message_method method; //required
    int region; //required
    void *data; //byte stream - optional
    int size; //optional
    bool status; //optional 
    
} clipboard_message;

typedef struct packed_message {
    void *buf;
    int size;
} packed_message;

packed_message new_response(message_method method, int region, void *data, size_t count, bool status);
packed_message new_size_message(message_method method, int region, size_t count);
packed_message new_request(message_method method, int region, void *data, size_t count);

int clipboard_connect(char *clipboard_dir);
int clipboard_copy(int clipboard_id, int region, void *buf, size_t count);
int clipboard_paste(int clipboard_id, int region, void *buf, size_t count);
int clipboard_wait(int clipboard_id, int region, void *buf, size_t count);
void clipboard_close(int clipboard_id);

bool validate_region(int region);
#endif