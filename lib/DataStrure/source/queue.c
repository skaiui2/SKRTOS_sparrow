#include "queue.h"

void queue_enqueue(struct list_node *head, struct list_node *node) 
{
    list_add_prev(head, node);
}

struct list_node *queue_dequeue(struct list_node *head) 
{
    if (list_empty(head)) return NULL;
    struct list_node *front = head->next;
    list_remove(front);
    return front;
}
