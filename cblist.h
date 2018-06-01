#ifndef CBLIST_H
#define CBLIST_H

#include <pthread.h>
#include <stdlib.h>

typedef struct cb_client
{
    size_t id;
    pthread_t thread_id;
    size_t socket_fd;
    size_t socket_teste;
    struct cb_client *next;
} cb_client;

typedef struct connected_list
{
    cb_client *cb;
    size_t size;
    pthread_mutex_t mutex;
} connected_list;

connected_list *new_list();
cb_client *new_clipboard(size_t socket_fd, size_t socket_teste);
void add_clipboard(connected_list *list, cb_client *cb);
void remove_clipboard_by_thread_id(connected_list *list, pthread_t thread_id);
void free_list(connected_list *list);

#endif