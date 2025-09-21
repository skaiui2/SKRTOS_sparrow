#include "stack.h"

void stack_push(struct list_node *head, struct list_node *node) 
{
    list_add_next(head, node);
}

struct list_node *stack_pop(struct list_node *head) 
{
    if (list_empty(head)) return NULL;
    struct list_node *top = head->next;
    list_remove(top);
    return top;
}
