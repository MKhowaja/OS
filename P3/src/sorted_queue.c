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

int queue_add(queue* list, void* data) {
		queue_node* prevNode;
		queue_node* new_node;
		MSG_BUF* tempMessageNode;
		MSG_BUF* newMessageNode;
    if (list == NULL) {
        return 1;
    }
		new_node->next = NULL;
		new_node->prev = NULL;
		new_node->value = data;
		list->length++;
		prevNode = list->first;
		if (prevNode == NULL){
				list->first = new_node;
				list->last = new_node;
				return 0;
		}
		tempMessageNode = (MSG_BUF*) prevNode->value;
		newMessageNode = (MSG_BUF*)new_node->value;
		if (newMessageNode->msg_delay < tempMessageNode->msg_delay){
				new_node->next = list->first;
				list->first = new_node;
		}
		else {
				tempMessageNode = (MSG_BUF*) prevNode->next->value;
				while (tempMessageNode!=NULL && tempMessageNode->msg_delay < newMessageNode->msg_delay){
						prevNode= prevNode->next;
						tempMessageNode = (MSG_BUF*) prevNode->next->value;
				}
				new_node->next = prevNode->next;
				prevNode->next = new_node;
				if (new_node->next == NULL){
						list->last = new_node;
				}
		}
    
    return 0;
}

void* queue_pop_front(queue* list) {
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

    return first_node->value;
}

void* peek (queue* list) {
		return list->first->value;
}
