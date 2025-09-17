#ifndef QUEUE_H
#define QUEUE_H

#include "link_list.h"

void queue_enqueue(struct list_node *head, struct list_node *node);

struct list_node *queue_dequeue(struct list_node *head);

#endif 
