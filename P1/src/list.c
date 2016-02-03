#include "list.h"

int linkedList_init(linkedList* list) {
    if (list == NULL) {
        return 1;
    }

    list->first = NULL;
    list->last = NULL;
    list->length = 0;

    return 0;
}

int linkedList_push_front(linkedList* list, node* new_node) {
    if (list == NULL) {
        return 1;
    }

    if (list->first != NULL) {
        list->first->prev = new_node;
    }

    new_node->next = list->first;
    new_node->prev = NULL; // whoever gave me this node is responsible for managing the new_node->prev pointer

    list->length++;
    list->first = new_node;

    if (list->last == NULL) {
        list->last = new_node;
    }

    return 0;
}

int linkedList_push_back(linkedList* list, node* new_node) {
    if (list == NULL) {
        return 1;
    }

    if (list->last != NULL) {
        list->last->next = new_node;
    }

    new_node->prev = list->last;
    new_node->next = NULL; // whoever gave me this node is responsible for managing the new_node->next pointer

    list->length++;
    list->last = new_node;

    if (list->first == NULL) {
        list->first = new_node;
    }

    return 0;
}

node* linkedList_pop_front(linkedList* list) {
    node* first_node;
    node* second_node;

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

node* linkedList_pop_back(linkedList* list) {
    node* last_node;
    node* second_last_node;

    if (list == NULL || list->last == NULL) {
        return NULL;
    }
    // guarantees list->last is not null

    last_node        = list->last;
    second_last_node = last_node->prev;

    if (second_last_node != NULL) {
        second_last_node->next = NULL;
    }

    list->last = second_last_node;

    if (list->last == NULL) {
        list->first = NULL;
    }

    list->length--;

    return last_node;
}

// node* linkedList_remove(linkedList* list, void* target_value) {
//     node* iter;

//     if (list == NULL) {
//         return NULL;
//     }

//     if (list->first != NULL && list->first->value == target_value) {
//         return (node*)linkedList_pop_front(list);
//     }

//     if (list->last != NULL && list->last->value == target_value) {
//         return (node*)linkedList_pop_back(list);
//     }

//     iter = list->first;

//     while (iter != NULL) {
//         if (iter->value == target_value) {
//             if (iter->next != NULL) {
//                 iter->next->prev = iter->prev;
//             }

//             if (iter->prev != NULL) {
//                 iter->prev->next = iter->next;
//             }

//             list->length--;

//             return iter;
//         }

//         iter = iter->next;
//     }

//     return NULL;
// }
