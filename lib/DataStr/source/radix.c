#include "radix.h"


struct radix_tree_node {
    unsigned int height;
    unsigned int count;
    void *slots[SIZE_LEVEL];
};

struct radix_tree_root {
    unsigned int height;
    struct radix_tree_node *rnode;
};


void radix_tree_init(struct radix_tree_root *root)
{
    root->height = 0;
    root->rnode = NULL;
}


struct radix_tree_node *radix_tree_node_alloc(unsigned int height)
{
    struct radix_tree_node *node = heap_malloc(sizeof(struct radix_tree_node));
    if (node == NULL) {
        return NULL;
    }
    *node = (struct radix_tree_node) {
        .height = height,
        .count  = 0,
    };

    return node;
}



int radix_tree_insert(struct radix_tree_root *root, unsigned long index, void *item)
{
    struct radix_tree_node **node_ptr = &root->rnode;
    unsigned int height = root->height;
    unsigned long shift;
    int offset;

    if (!*node_ptr) {
        root->rnode = radix_tree_node_alloc(0);
        if (!root->rnode) {
            return -1;
        }
    }

    while (index >= (1UL << (BIT_LEVEL * (height + 1)))) {
        struct radix_tree_node *new_node = radix_tree_node_alloc(height + 1);
        if (!new_node) {
            return -1;
        }
        new_node->slots[0] = root->rnode;
        new_node->count = 1;
        root->rnode = new_node;
        root->height++;
        height++;
    }

    node_ptr = &root->rnode;
    shift = BIT_LEVEL * height;
    while (height > 0) {
        struct radix_tree_node *node = *node_ptr;
        offset = (int)((index >> shift) & (SIZE_LEVEL - 1));
        if (!node->slots[offset]) {
            node->slots[offset] = radix_tree_node_alloc(height - 1);
            if (!node->slots[offset]) {
                return -1;
            }
            node->count++;
        }
        node_ptr = (struct radix_tree_node **)&(node->slots[offset]);
        shift -= BIT_LEVEL;
        height--;
    }

    struct radix_tree_node *leaf = *node_ptr;
    offset = (int)(index & (SIZE_LEVEL - 1));
    if (leaf->slots[offset]) {
        fprintf(stderr, "Key already exists in the radix tree\n");
        return -1;
    }
    leaf->slots[offset] = item;
    leaf->count++;
    return 0;
}


void *radix_tree_lookup(struct radix_tree_root *root, unsigned long index)
{
    struct radix_tree_node *node = root->rnode;
    unsigned int height = root->height;
    unsigned long shift = BIT_LEVEL * height;
    int offset;

    while (height > 0) {
        if (!node) {
            return NULL;
        }
        offset = (int)((index >> shift) & (SIZE_LEVEL - 1));
        node = (struct radix_tree_node *)node->slots[offset];
        shift -= BIT_LEVEL;
        height--;
    }

    if (!node) {
        return NULL;
    }
    offset = (int)(index & (SIZE_LEVEL - 1));
    return node->slots[offset];
}


void *radix_tree_delete(struct radix_tree_root *root, unsigned long index)
{
    struct radix_tree_node *node = root->rnode;
    struct radix_tree_node *parent = NULL;
    int offset, parent_offset = 0;
    unsigned int height = root->height;
    unsigned long shift = BIT_LEVEL * height;
    
    while (height > 0) {
        if (!node) {
            return NULL;
        }
        offset = (int)((index >> shift) & (SIZE_LEVEL - 1));
        parent = node;
        parent_offset = offset;
        node = (struct radix_tree_node *)node->slots[offset];
        shift -= BIT_LEVEL;
        height--;
    }
    
    if (node == NULL) {
        return NULL;
    }
    offset = (int)(index & (SIZE_LEVEL - 1));
    void *item = node->slots[offset];
    if (item == NULL) {
        return NULL;
    }

    node->slots[offset] = NULL; 
    node->count--;

    while (parent && node->count == 0) {
        heap_free(node);
        parent->slots[parent_offset] = NULL;
        parent->count--;
        node = parent;
    }

    return item;
}

