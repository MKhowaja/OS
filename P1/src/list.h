#ifndef __LIST_H
#define __LIST_H
 
typedef struct node {
    struct node* next;
    struct node* prev;
    void* value;
} node_t;

typedef struct {
    node_t* first;
    node_t* last;
    int length;
} linkedlist_t;

int linkedlist_init(linkedlist_t* list);

int linkedlist_push_front(linkedlist_t* list, node_t* new_node);
int linkedlist_push_back(linkedlist_t* list, node_t* new_node);

node_t* linkedlist_pop_front(linkedlist_t* list);
node_t* linkedlist_pop_back(linkedlist_t* list);
node_t* linkedlist_remove(linkedlist_t* list, void* target_value);

#endif
