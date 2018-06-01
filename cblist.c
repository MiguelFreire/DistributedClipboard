#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include "cblist.h"
#include "utils.h"

connected_list *new_list()
{
    connected_list *list;
    list = scalloc(1, sizeof(connected_list)); //Init all values to default
    pthread_mutex_init(&(list->mutex), NULL);
    return list;
}

cb_client *new_clipboard(size_t socket_fd, size_t socket_teste)
{
    //TODO: validate arguments
    cb_client *cb;

    cb = scalloc(1, sizeof(cb_client));
    cb->socket_fd = socket_fd;
    cb->socket_teste = socket_teste;
    return cb;
}

void add_clipboard(connected_list *list, cb_client *cb)
{
    cb_client *aux;
    pthread_mutex_lock(&(list->mutex));
    if (list->cb == NULL)
    { //If no head exists add it to the head
        list->cb = cb;
        list->size++;
        cb->id = list->size;
        pthread_mutex_unlock(&(list->mutex));
        return;
    }

    aux = list->cb;
    while (aux->next != NULL)
        aux = aux->next; //go to the end of the list

    aux->next = cb;
    list->size++;
    pthread_mutex_unlock(&(list->mutex));
}

void remove_clipboard_by_thread_id(connected_list *list, pthread_t thread_id)
{
    cb_client *aux;
    cb_client *cur;
    bool found = false;
    pthread_mutex_lock(&(list->mutex));
    if (list->cb == NULL)
        return;
    cur = list->cb;
    //Current After
    //Next To be removed
    //Next next

    while (cur->next != NULL)
    {
        if (pthread_equal(cur->next->thread_id, thread_id) > 0)
        {
            found = true;
            break;
        }
        cur = cur->next;
    }
    if (found)
    {
        aux = cur->next; //to be removed
        cur->next = aux->next;

        free(aux);
        list->size--;
    }
    pthread_mutex_unlock(&(list->mutex));
}

void free_list(connected_list *list)
{
    cb_client *aux, *cur;
    pthread_mutex_lock(&(list->mutex));
    cur = list->cb;
    
    while (cur != NULL)
    {
        aux = cur;
        cur = aux->next;
        free(aux);
    }
    pthread_mutex_unlock(&(list->mutex));
    pthread_mutex_destroy(&(list->mutex));
    free(list);
}
