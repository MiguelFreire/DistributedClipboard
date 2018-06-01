#ifndef CLIPBOARD_PROTOCOL_H
#define CLIPBOARD_PROTOCOL_H
#include "cbmessage.pb-c.h"
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "utils.h"

#define CLIPBOARD_SOCKET "./CLIPBOARD_SOCKET"
#define MESSAGE_MAX_SIZE 1024
#define NUM_REGIONS 10

#define Request CBMESSAGE__TYPE__Request
#define Response CBMESSAGE__TYPE__Response
#define Copy CBMESSAGE__METHOD__Copy
#define Paste CBMESSAGE__METHOD__Paste
#define Sync CBMESSAGE__METHOD__Sync
#define Wait CBMESSAGE__METHOD__Wait

typedef CBMessage__Type message_type;
typedef CBMessage__Method message_method;

typedef struct packed_message
{
    void *buf;
    int size;
} packed_message;

packed_message new_message(message_type type, message_method method, int region, void *data, size_t count, bool has_status, bool status, bool has_lower_copy, bool lower_copy);

#endif