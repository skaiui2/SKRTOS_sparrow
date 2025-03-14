/*
 * MIT License
 *
 * Copyright (c) 2024 skaiui2

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *  https://github.com/skaiui2/SKRTOS_sparrow
 */

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
    uint8_t rb_color;
    rb_node *rb_right;
    rb_node *rb_left;
    uint64_t value;
};

Class(rb_root)
{
    rb_node *rb_node;
    rb_node *first_node;
    rb_node *last_node;
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

