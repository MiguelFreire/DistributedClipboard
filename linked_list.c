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
    if(head == NULL) return NULL;

    

    
}


linked_list_node *add_item_at_head(linked_list *head, linked_list_node *node) {
    
}
linked_list_node *add_item_at_end(linked_list *head, void *data);
void free_linked_list(linked_list *head, void (* freeNode)(void*data));