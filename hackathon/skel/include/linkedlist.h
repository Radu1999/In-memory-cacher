#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <pthread.h>

struct Node {
    void *data;
    struct Node *next;
};

struct LinkedList {
    struct Node *head;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t is_available;
};

void init_list(struct LinkedList *list);

void add_nth_node(struct LinkedList *list, int n, void *new_data);

struct Node* remove_nth_node(struct LinkedList *list, int n);

int get_size(struct LinkedList *list);

void free_list(struct LinkedList **pp_list);


#endif