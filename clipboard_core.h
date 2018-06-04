#ifndef CLIPBOARD_CORE_H
#define CLIPBOARD_CORE_H
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "clipboard_protocol.h"
#include "utils.h"
#include "cblist.h"


typedef struct store_object
{
    void *data;
    size_t size;
} store_object;

typedef enum client_t
{
    App,
    Clipboard,
    Parent
} client_t;

int clipboard_lower_copy(int clipboard_id, int region, void *buf, size_t count);
packed_message new_sync_message();
int cbstore(size_t region, void *data, size_t count);
int clipboard_sync(int clipboard_id);
bool validate_region(int region);
#endif