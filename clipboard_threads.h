#ifndef CLIPBOARD_THREADS_H
#define CLIPBOARD_THREADS_H
#include "cblist.h"
#include "clipboard_core.h"
#include "clipboard_com.h"
#include "clipboard.h"
#include "clipboard_handler.h"

typedef struct thread_arg
{
    int client;
    client_t type;
} thread_arg;

void *thread_lower_com(void *arg);
void *thread_upper_com(void *arg);
void *thread_client(void *arg);
void *thread_inet_handler(void *arg);
#endif