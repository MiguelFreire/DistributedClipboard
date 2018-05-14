#include <stdlib.h>
#include <stdio.h>

#include "linked_list.h"
#include "utils.h"

linked_list new_linked_list() {
    linked_list list = {0 , NULL};

    return list;
}
linked_list_node *new_item(void *data) {
    linked_list_node *item;

    item = smalloc(sizeof(linked_list_node));
    item->data = data;
    item->next = NULL;
    
    return item;   
}

linked_list_node *add_node(linked_list *list, linked_list_node *node) {
    if(list->head == NULL) {
        list->head = node;
        return list->head;
    }
    linked_list_node *aux;
    aux = head;
    while(aux->next != NULL) aux = aux->next;

    //Find last node
    aux->next = node;

    list->size++;

    return node;    
}

void free_linked_list(linked_list *list) {
    linked_list_node *aux, *cur;
    cur = list->head;
    while(cur != NULL) {
        aux = cur;
        cur = aux->next;

        free(aux->data);

    }
}