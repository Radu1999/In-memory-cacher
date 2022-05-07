#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/linkedlist.h"

void init_list(struct LinkedList *list) {
    list->head = NULL;
    pthread_mutex_init(&list->lock, NULL);
    pthread_cond_init(&list->is_available, NULL);
    list->count = 0;

}

void add_nth_node(struct LinkedList *list, int n, void *new_data) {
    if(n < 0) {
        return ;
    }
    struct Node *nod = malloc(sizeof(struct Node));
    nod->data = new_data;
    int position;
    if(!list->count) {
        nod->next = NULL;
        list->head = nod;
    }
    else {

        if(n >= list->count) {
            n = list->count;
        }
        if(n) {
            struct Node *prev;
            position = 0;
            prev = list->head;
            while(position++ < n - 1) {
                prev = prev->next;
            }
            nod->next = prev->next;
            prev->next = nod;

        }
        else {
            nod->next = list->head;
            list->head = nod;
        }
       
    }
    list->count++;
    
}

struct Node* remove_nth_node(struct LinkedList *list, int n) {
    struct Node *it, *prev;
    struct Node *ans = malloc(sizeof(struct Node));
    if(n < 0 || list->count == 0) {
        return NULL;
    }
    if(list -> count == 1) {
        ans->next = list->head->next;
        ans->data = list->head->data;
        list -> head = NULL;
    }
    else {

        if(n >= list->count) {
            n = list->count - 1;
        }
        if(n) {
            int position = 0;
            it = list -> head;
            while(position++ < n) {
                prev = it;
                it = it->next;
            }
            ans->next = prev->next->next;
            ans->data = prev->next->data;
            prev -> next = it->next;
        }
        else {
            ans->next = list->head->next;
            ans->data = list->head->data;
            list->head = list ->head -> next; 
        }
    }
    list -> count--;
    return ans;
}

int get_size(struct LinkedList *list) {
    return list->count;
}

void free_list(struct LinkedList **pp_list) {

    struct Node *it;
    struct Node *prev;
    it = (*pp_list)->head;
    while(it != NULL) {
        prev = it;
        it = it->next;
        free(prev);
    }
    free(*pp_list);
}

void print_int_linkedlist(struct LinkedList *list) {
    struct Node *it;
    it = list->head;
    while(it != NULL) {
        printf("%d ", *((int*)it->data));
        it = it->next;
    }
    printf("\n");
}
