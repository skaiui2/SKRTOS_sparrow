

#include "link_list.h"

void list_node_init(struct list_node *node)
{
    *node = (struct list_node) {
            .next = node,
            .prev = node
    };
}

/*
 * prev insert in front of next.
*/
void list_add_prev(struct list_node *next, struct list_node *prev)
{
    prev->prev = next->prev;
    prev->next = next;
    next->prev->next = prev;
    next->prev = prev;
}

void list_add_next(struct list_node *prev, struct list_node *next)
{
    next->prev = prev;
    next->next = prev->next;
    prev->next->prev = next;
    prev->next = next;

}



void list_remove(struct list_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}
