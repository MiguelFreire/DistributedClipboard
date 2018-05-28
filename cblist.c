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

    if (list->cb == NULL)
    { //If no head exists add it to the head
        list->cb = cb;
        list->size++;
        cb->id = list->size;
        return;
    }

    aux = list->cb;
    while (aux->next != NULL)
        aux = aux->next; //go to the end of the list

    aux->next = cb;
    list->size++;
}

void remove_clipboard_by_thread_id(connected_list *list, pthread_t thread_id)
{
    cb_client *aux;
    cb_client *cur;
    bool found = false;
    if (list->cb == NULL)
        return;
    cur = list->cb;
    //Current After
    //Next To be removed
    //Next next

    while (cur->next != NULL)
    {
        if (cur->next->thread_id == thread_id)
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
}

void free_list(connected_list *list)
{
    cb_client *aux, *cur;

    cur = list->cb;

    while (cur != NULL)
    {
        aux = cur;
        cur = aux->next;
        free(aux);
    }

    free(list);
}
