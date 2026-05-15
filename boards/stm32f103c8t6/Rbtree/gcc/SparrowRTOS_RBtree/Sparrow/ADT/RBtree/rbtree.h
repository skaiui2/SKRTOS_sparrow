

#ifndef RBTREE_H
#define RBTREE_H
#include "class.h"

#define	RB_RED		0
#define	RB_BLACK	1

typedef struct rb_node * rb_node_handle;
typedef struct rb_root * rb_root_handle;

Class(rb_node)
{
    rb_node *rb_parent;
    rb_root_handle root;
    uint8_t rb_color : 1;
    rb_node *rb_right;
    rb_node *rb_left;
    uint32_t value;
};

Class(rb_root)
{
    rb_node *rb_node;
    rb_node *save_node;
    uint32_t count;
};


void rb_root_init(rb_root_handle root);
void rb_node_init(rb_node_handle node);
void rb_Insert_node(rb_root_handle root,  rb_node *new_node);
void rb_remove_node(rb_root *root,  rb_node *node);


rb_node_handle rb_first(rb_root_handle root);
rb_node_handle rb_last(rb_root_handle root);
rb_node_handle rb_next(rb_node_handle node);
rb_node_handle rb_prev(rb_node_handle node);

void rb_replace_node(rb_node_handle victim,  rb_node_handle new,
                      rb_root_handle root);




#endif

