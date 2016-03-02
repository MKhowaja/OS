#include "sorted_queue.h"
#include "k_rtx.h"

int queue_init(queue* list) {
    if (list == NULL) {
        return 1;
    }

    list->first = NULL;
    list->last = NULL;
    list->length = 0;

    return 0;
}

int queue_add(queue* list, queue_node* new_node) {
		queue_node* prevNode;
		MSG_T* tempMessageNode;
		MSG_T* newMessageNode;
    if (list == NULL) {
        return 1;
    }
		new_node->next = NULL;
		new_node->prev = NULL;
		list->length++;
		prevNode = list->first;
		if (prevNode == NULL){
				list->first = new_node;
				list->last = new_node;
				return 0;
		}
		tempMessageNode = (MSG_T*) prevNode;
		newMessageNode = (MSG_T*)new_node;
		if (newMessageNode->msg_delay < tempMessageNode->msg_delay){
				new_node->next = list->first;
				list->first = new_node;
		}
		else {
				tempMessageNode = (MSG_T*) prevNode->next;
				while (tempMessageNode!=NULL && tempMessageNode->msg_delay < newMessageNode->msg_delay){
						prevNode= prevNode->next;
						tempMessageNode = (MSG_T*) prevNode->next;
				}
				new_node->next = prevNode->next;
				prevNode->next = new_node;
				if (new_node->next == NULL){
						list->last = new_node;
				}
		}
    
    return 0;
}

queue_node* queue_pop_front(queue* list) {
		queue_node* first_node;
    queue_node* second_node;

    if (list == NULL || list->first == NULL) {
        return NULL;
    }
    // guarantees list->first is not null

    first_node  = list->first;
    second_node = first_node->next;

    if (second_node != NULL) {
        second_node->prev = NULL;
    }

    list->first = second_node;

    if (list->first == NULL) {
        list->last = NULL;
    }

    list->length--;

    return first_node;
}

queue_node* peek (queue* list) {
		return list->first;
}
