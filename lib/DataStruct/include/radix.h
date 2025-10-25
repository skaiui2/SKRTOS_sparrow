#ifndef RADIX_H
#define RADIX_H
#include "macro.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

struct radix_tree_node {
    void *slots[2];
    struct radix_tree_node *parent;
    uint8_t offset;
    unsigned int count;
    unsigned int height;
};

struct radix_tree_root {
    unsigned int height;
    unsigned int count;
    struct radix_tree_node *rnode;
};


void radix_tree_init(struct radix_tree_root *root);
void radix_tree_node_init(struct radix_tree_node *node, uint8_t height);
int radix_tree_insert(struct radix_tree_root *root, size_t index, void *item);
void *radix_tree_lookup(struct radix_tree_root *root, size_t index);
void *radix_tree_root_left(struct radix_tree_root *root);
void *radix_tree_lookup_upper_bound(struct radix_tree_root *root, size_t index);
void *radix_tree_delete(struct radix_tree_root *root, unsigned long index);




#endif 
