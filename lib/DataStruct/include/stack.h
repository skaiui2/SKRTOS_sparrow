#ifndef STACK_H
#define STACK_H

#include "link_list.h"

void stack_push(struct list_node *head, struct list_node *node);

struct list_node *stack_pop(struct list_node *head);

#endif 
