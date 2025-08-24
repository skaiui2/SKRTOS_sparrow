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

#include "rbtree.h"


static inline void rb_link_node(rb_node *node,  rb_node *parent,
                                rb_node **rb_link)
{
    node->rb_parent = parent;
    node->rb_color = RB_RED;
    node->rb_left = node->rb_right = NULL;

    *rb_link = node;
}


static inline void rb_rotate_left(rb_node *node,  rb_root *root)
{
    rb_node *right = node->rb_right;

    node->rb_right = right->rb_left;
    if (node->rb_right != NULL) {
        right->rb_left->rb_parent = node;
    }
    right->rb_left = node;

    right->rb_parent = node->rb_parent;
    if (right->rb_parent != NULL) {
        if (node == node->rb_parent->rb_left) {
            node->rb_parent->rb_left = right;
        } else {
            node->rb_parent->rb_right = right;
        }
    } else {
        root->rb_node = right;
    }

    node->rb_parent = right;
}

static void rb_rotate_right(rb_node *node,  rb_root *root)
{
    rb_node *left = node->rb_left;

    node->rb_left = left->rb_right;
    if (node->rb_left != NULL) {
        left->rb_right->rb_parent = node;
    }
    left->rb_right = node;

    left->rb_parent = node->rb_parent;
    if (left->rb_parent != NULL) {
        if (node == node->rb_parent->rb_right) {
            node->rb_parent->rb_right = left;
        } else {
            node->rb_parent->rb_left = left;
        }
    } else {
        root->rb_node = left;
    }

    node->rb_parent = left;
}


static void rb_insert_color(rb_node *node,  rb_root *root)
{
    rb_node *parent, *grand_parent;

    while ((parent = node->rb_parent) && parent->rb_color == RB_RED) {
        grand_parent = parent->rb_parent;

        if (parent == grand_parent->rb_left) {

            register rb_node *uncle = grand_parent->rb_right;
            if (uncle && uncle->rb_color == RB_RED) {
                uncle->rb_color = RB_BLACK;
                parent->rb_color = RB_BLACK;
                grand_parent->rb_color = RB_RED;
                node = grand_parent;
                continue;
            }

            if (parent->rb_right == node) {
                register rb_node *tmp;
                rb_rotate_left(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->rb_color = RB_BLACK;
            grand_parent->rb_color = RB_RED;
            rb_rotate_right(grand_parent, root);
        } else {
            register rb_node *uncle = grand_parent->rb_left;
            if (uncle && uncle->rb_color == RB_RED) {
                uncle->rb_color = RB_BLACK;
                parent->rb_color = RB_BLACK;
                grand_parent->rb_color = RB_RED;
                node = grand_parent;
                continue;
            }

            if (parent->rb_left == node) {
                register rb_node *tmp;
                rb_rotate_right(parent, root);
                tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->rb_color = RB_BLACK;
            grand_parent->rb_color = RB_RED;
            rb_rotate_left(grand_parent, root);
        }
    }

    root->rb_node->rb_color = RB_BLACK;
}


static void rb_erase_color( rb_node *node,  rb_node *parent,
                            rb_root *root)
{
    rb_node *other;

    while ((!node || node->rb_color == RB_BLACK) && node != root->rb_node) {
        if (parent->rb_left == node) {
            other = parent->rb_right;
            if (other->rb_color == RB_RED) {
                other->rb_color = RB_BLACK;
                parent->rb_color = RB_RED;
                rb_rotate_left(parent, root);
                other = parent->rb_right;
            }

            if ((!other->rb_left ||
                 other->rb_left->rb_color == RB_BLACK)
                && (!other->rb_right ||
                    other->rb_right->rb_color == RB_BLACK)) {
                other->rb_color = RB_RED;
                node = parent;
                parent = node->rb_parent;
            } else {
                if (!other->rb_right ||
                    other->rb_right->rb_color == RB_BLACK) {
                    register rb_node *o_left;
                    if ((o_left = other->rb_left)) {
                        o_left->rb_color = RB_BLACK;
                    }
                    other->rb_color = RB_RED;
                    rb_rotate_right(other, root);
                    other = parent->rb_right;
                }
                other->rb_color = parent->rb_color;
                parent->rb_color = RB_BLACK;
                if (other->rb_right) {
                    other->rb_right->rb_color = RB_BLACK;
                }
                rb_rotate_left(parent, root);
                node = root->rb_node;
                break;
            }
        } else {
            other = parent->rb_left;
            if (other->rb_color == RB_RED) {
                other->rb_color = RB_BLACK;
                parent->rb_color = RB_RED;
                rb_rotate_right(parent, root);
                other = parent->rb_left;
            }

            if ((!other->rb_left ||
                 other->rb_left->rb_color == RB_BLACK)
                && (!other->rb_right ||
                    other->rb_right->rb_color == RB_BLACK)) {
                other->rb_color = RB_RED;
                node = parent;
                parent = node->rb_parent;
            } else {
                if (!other->rb_left ||
                    other->rb_left->rb_color == RB_BLACK) {
                    register rb_node *o_right;
                    if ((o_right = other->rb_right)) {
                        o_right->rb_color = RB_BLACK;
                    }
                    other->rb_color = RB_RED;
                    rb_rotate_left(other, root);
                    other = parent->rb_left;
                }
                other->rb_color = parent->rb_color;
                parent->rb_color = RB_BLACK;
                if (other->rb_left) {
                    other->rb_left->rb_color = RB_BLACK;
                }
                rb_rotate_right(parent, root);
                node = root->rb_node;
                break;
            }
        }
    }

    if (node) {
        node->rb_color = RB_BLACK;
    }

}

static void rb_erase(rb_node *node,  rb_root *root)
{
    rb_node *child, *parent;
    int color;

    if (!node->rb_left) {
        child = node->rb_right;
    }
    else if (!node->rb_right) {
        child = node->rb_left;
    }
    else {
        rb_node *left;
        rb_node *old = node;

        node = node->rb_right;
        while ((left = node->rb_left) != NULL) {
            node = left;
        }

        child = node->rb_right;
        parent = node->rb_parent;
        color = node->rb_color;

        if (child) {
            child->rb_parent = parent;
        }
        if (parent) {
            if (parent->rb_left == node) {
                parent->rb_left = child;
            } else {
                parent->rb_right = child;
            }
        } else {
            root->rb_node = child;
        }

        if (node->rb_parent == old) {
            parent = node;
        }
        node->rb_parent = old->rb_parent;
        node->rb_color = old->rb_color;
        node->rb_right = old->rb_right;
        node->rb_left = old->rb_left;

        if (old->rb_parent) {
            if (old->rb_parent->rb_left == old) {
                old->rb_parent->rb_left = node;
            } else {
                old->rb_parent->rb_right = node;
            }
        } else {
            root->rb_node = node;
        }

        old->rb_left->rb_parent = node;
        if (old->rb_right) {
            old->rb_right->rb_parent = node;
        }
        goto color;
    }

    parent = node->rb_parent;
    color = node->rb_color;

    if (child) {
        child->rb_parent = parent;
    }
    if (parent) {
        if (parent->rb_left == node) {
            parent->rb_left = child;
        } else {
            parent->rb_right = child;
        }
    } else {
        root->rb_node = child;
    }

    color:
    if (color == RB_BLACK) {
        rb_erase_color(child, parent, root);
    }
}



rb_node *rb_first(rb_root *root)
{
    rb_node	*n;
    n = root->rb_node;
    if (!n) {
        return NULL;
    }
    while (n->rb_left) {
        n = n->rb_left;
    }
    return n;
}


rb_node *rb_last(rb_root *root)
{
    rb_node	*n;

    n = root->rb_node;
    if (!n) {
        return NULL;
    }
    while (n->rb_right) {
        n = n->rb_right;
    }
    return n;
}

/*
 * if right node be, return it.
 * if no, return successor node.
 */

rb_node *rb_next(rb_node *node)
{
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left) {
            node = node->rb_left;
        }
        return node;
    }

    while (node->rb_parent && node == node->rb_parent->rb_right) {
        node = node->rb_parent;
    }

    return node->rb_parent;
}

/*
 * If we have a left-hand child, go down and then right as far
 *  as we can.
 * No left-hand children. Go up till we find an ancestor which
 * is a right-hand child of its parent
 */

rb_node *rb_prev(rb_node *node)
{
    if (node->rb_left) {
        node = node->rb_left;
        while (node->rb_right) {
            node = node->rb_right;
        }
        return node;
    }

    while (node->rb_parent && node == node->rb_parent->rb_left) {
        node = node->rb_parent;
    }

    return node->rb_parent;
}


void rb_replace_node( rb_node *victim,  rb_node *new,
                      rb_root *root)
{
    rb_node *parent = victim->rb_parent;

    if (parent) {
        if (victim == parent->rb_left)
            parent->rb_left = new;
        else
            parent->rb_right = new;
    } else {
        root->rb_node = new;
    }
    if (victim->rb_left)
        victim->rb_left->rb_parent = new;
    if (victim->rb_right)
        victim->rb_right->rb_parent = new;

    *new = *victim;
}


rb_node *rb_first_greater(rb_root *root, size_t key)
{
    rb_node *node = root->rb_node;
    rb_node *candidate = NULL;

    while (node != NULL) {
        if (node->value > key) {
            candidate = node;
            node = node->rb_left;
        } else {
            node = node->rb_right;
        }
    }

    return candidate;
}

void rb_root_init(rb_root_handle root)
{
    *root = (rb_root){
        .rb_node = NULL,
        .first_node = NULL,
        .last_node = NULL,
        .count = 0
    };
}


void rb_node_init(rb_node_handle node)
{
    *node = (rb_node){
            .rb_left = NULL,
            .rb_color = RB_RED,
            .rb_right = NULL,
            .rb_parent = NULL,
            .value = 0,
            .root = NULL
    };
}

/*
 * The nodes that are added subsequently are positioned at the rear of the tree structure.
 */
void rb_Insert_node(rb_root_handle root,  rb_node *new_node) {
    rb_node **link = &(root->rb_node), *parent = NULL;

    while (*link) {
        parent = *link;
        if (new_node->value < (*link)->value) {
            link = &((*link)->rb_left);
        } else {
            link = &((*link)->rb_right);
        }
    }

    if (root->count != 0) {
        if (new_node->value >= root->last_node->value) {
            root->last_node = new_node;
        }

        if (new_node->value < root->first_node->value) {
            root->first_node = new_node;
        }
    } else {
        root->first_node = new_node;
        root->last_node = new_node;
    }

    rb_link_node(new_node, parent, link);
    rb_insert_color(new_node, root);
    root->count++;
}


void rb_remove_node(rb_root *root,  rb_node *node)
{
    if (root->count > 1) {
        if (node == root->last_node) {
            root->last_node = rb_prev(node);
        }
        if (node == root->first_node) {
            root->first_node = rb_next(node);
        }
    } else {
        root->last_node = NULL;
        root->first_node = NULL;
    }
    rb_erase(node, root);
    root->count--;
}


rb_node *search_node(rb_root *root, uint32_t key) {
    rb_node *node = root->rb_node;

    while (node) {
        if (key < node->value) {
            node = node->rb_left;
        }
        else if (key > node->value) {
            node = node->rb_right;
        }
        else {
            return node;
        }
    }
    return NULL;
}



