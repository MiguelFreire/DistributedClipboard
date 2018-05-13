#ifndef LINKEDLIST_H
#define LINKEDLIST_H

typedef struct _linked_list {
    size_t size;
    linked_list_node *head;
} linked_list;

typedef struct _linked_list_node {
    void *data;
    linked_list_node *next;
} _linked_list_node;

linked_list new_linked_list();
linked_list node *new_item();
linked_list_node *add_node(linked_list *head, linked_list_node *node);
linked_list_node *add_item_at_head(linked_list *head, void *data);
linked_list_node *add_item_at_end(linked_list *head, void *data);
void free_linked_list(linked_list *head, void (* freeNode)(void*data));

#endif