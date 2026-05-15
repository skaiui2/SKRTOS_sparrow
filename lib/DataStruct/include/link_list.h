
#ifndef LINK_LIST_H
#define LINK_LIST_H


/*
 * Base list struct.
 */
struct list_node {
    struct list_node *next;
    struct list_node *prev;
};


void list_node_init(struct list_node *node);

int list_empty(struct list_node *node);

void list_add_prev(struct list_node *next, struct list_node *prev);

void list_add_next(struct list_node *prev, struct list_node *next);

void list_remove(struct list_node *node);




#endif
