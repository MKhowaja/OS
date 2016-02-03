#ifndef __LIST_H
#define __LIST_H

typedef struct _node {
    struct _node* next;
    struct _node* prev;
    void* value;
} node;

typedef struct {
    node* first;
    node* last;
    int length;
} linkedList;

int linkedList_init(linkedList* list);

int linkedList_push_front(linkedList* list, node* new_node);
int linkedList_push_back(linkedList* list, node* new_node);

node* linkedList_pop_front(linkedList* list);
node* linkedList_pop_back(linkedList* list);
node* linkedList_remove(linkedList* list, void* target_value);

#endif
