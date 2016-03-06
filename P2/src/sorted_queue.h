#ifndef __SORTEDQUEUE_H
#define __SORTEDQUEUE_H

#define NULL 0

typedef struct q_node {
    struct q_node* next;
    struct q_node* prev;
    void* value;
} queue_node;

typedef struct {
    queue_node* first;
    queue_node* last;
    int length;
} queue;

int queue_init(queue* list);
int queue_add(queue* list, void* data); //node has value to be inserted in sorted order
void* queue_pop_front(queue* list);
void* peek (queue* list);

#endif
