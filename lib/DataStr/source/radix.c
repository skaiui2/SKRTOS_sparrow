#include "radix.h"


__attribute__( ( always_inline ) ) inline uint8_t log2_clz(uint32_t Table)
{
    return 31 - __builtin_clz(Table);
}


void radix_tree_init(struct radix_tree_root *root)
{
    root->count = 0;
    root->max_size = 0;
    root->height = 0;
    root->rnode = NULL;
}

void radix_tree_node_init(struct radix_tree_node *node, uint8_t height)
{
    *node = (struct radix_tree_node) {
            .height = height,
            .count  = 0,
    };
}

struct radix_tree_node *radix_tree_node_alloc(struct radix_tree_root *root, uint8_t height)
{
    struct radix_tree_node *node = heap_malloc(sizeof(struct radix_tree_node));//It can replace by membitpool
    if (node == NULL) {
        return NULL;
    }
    radix_tree_node_init(node, height);
    root->count++;

    return node;
}

int radix_tree_grow(struct radix_tree_root *root, size_t index, void *item)
{
    struct radix_tree_node **node_ptr = &root->rnode;
    struct radix_tree_node *node;
    uint8_t height;
    uint8_t shift;
    uint8_t offset;

    height = log2_clz(index);
    uint8_t out = height / BIT_LEVEL;
    uint8_t temp = out * BIT_LEVEL;
    if (temp == height) {
        height = out;
    } else {
        height = out + 1;
    }

    root->height = height;
    root->rnode->height = height + 1;
    shift = BIT_LEVEL * height;

    while (height > 0) {
        node = *node_ptr;
        offset = (int)((index >> shift) & (SIZE_LEVEL - 1));
        if (!node->slots[offset]) {
            node->slots[offset] = radix_tree_node_alloc(root, height);
            if (!node->slots[offset]) {
                return -1;
            }
            ((struct radix_tree_node *)node->slots[offset])->offset = offset;
            ((struct radix_tree_node *)node->slots[offset])->parent = node;
            node->count++;
        }
        node_ptr = (struct radix_tree_node **)&node->slots[offset];
        shift -= BIT_LEVEL;
        height--;
    }

    struct radix_tree_node *leaf = *node_ptr;
    offset = (int)(index & (SIZE_LEVEL - 1));
    if (leaf->slots[offset]) {
        return -1;
    }
    leaf->slots[offset] = item;
    leaf->count += 1;

    if (root->max_size < index) {
        root->max_size = index;
    }

    return 0;
}


int radix_tree_insert(struct radix_tree_root *root, size_t index, void *item)
{
    struct radix_tree_node **node_ptr = &root->rnode;
    uint8_t height;

    if (!*node_ptr) {
        *node_ptr = radix_tree_node_alloc(root, 0);
        if (!*node_ptr) {
            return -1;
        }
        return radix_tree_grow(root, index, item);
    }

    height = root->height;
    while (index >= (1UL << (BIT_LEVEL * (height + 1)))) {
        struct radix_tree_node *new_node = radix_tree_node_alloc(root, height + 1);
        if (!new_node) {
            return -1;
        }

        for (char i = 0; i < SIZE_LEVEL; i++) {
            if (root->rnode->slots[i]) {
                ((struct radix_tree_node *)root->rnode->slots[i])->parent = new_node;
            }
        }
        *new_node = *root->rnode;
        for (char i = 0; i < SIZE_LEVEL; i++) {
            if (root->rnode->slots[i]) {
                root->rnode->slots[i] = NULL;
            }
        }
        root->rnode->slots[0] = new_node;
        root->height++;
        height++;
    }

    return radix_tree_grow(root, index, item);
}


void *radix_tree_lookup(struct radix_tree_root *root, size_t index)
{
    struct radix_tree_node *node = root->rnode;
    uint8_t height = root->height;
    uint8_t shift = BIT_LEVEL * height;
    uint8_t offset;

    if (!node) {
        return NULL;
    }

    if (index > root->max_size) {
        return NULL;
    }

    height += 1;
    while (height > 0) {
        offset = (int)((index >> shift) & (SIZE_LEVEL - 1));
        node = (struct radix_tree_node *)node->slots[offset];
        if (!node) {
            return NULL;
        }
        shift -= BIT_LEVEL;
        height--;
    }

    return node;
}

void *go_back_index(uint8_t offset, struct radix_tree_node *node, unsigned long index, uint8_t shift)
{
    while ((offset >= SIZE_LEVEL) || (!node->slots[offset])) {
        node = node->parent;
        shift += BIT_LEVEL;
        offset = ((index >> shift) & (SIZE_LEVEL - 1)) + 1;
        while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
            offset++;
        }
        if ((offset < SIZE_LEVEL) && node->slots[offset]) {
            node = node->slots[offset];
            break;
        }
    }
    return node;
}



void *radix_tree_node_left(struct radix_tree_node *node)
{
    int offset;
    uint8_t height;
    if (!node) {
        return NULL;
    }

    height = node->height;
    while (height > 0) {
        if (!node) {
            return NULL;
        }
        offset = 0;
        while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
            offset++;
        }
        node = (struct radix_tree_node *)node->slots[offset];
        height--;
    }
    return node;
}




__attribute__((always_inline)) inline void *radix_tree_root_left(struct radix_tree_root *root)
{
    return radix_tree_node_left(root->rnode);
}


void *radix_tree_lookup_upper_bound(struct radix_tree_root *root, size_t index) {
    struct radix_tree_node *node = root->rnode;
    struct radix_tree_node *parent;
    uint8_t height = root->height;
    uint8_t shift = BIT_LEVEL * height;
    uint8_t offset;
    uint8_t fit_search = 0;

    if (index > root->max_size) {
        return NULL;
    }

    height += 1;
    while (height > 0) {
        if (!node) {
            return NULL;
        }
        offset = (int)((index >> shift) & (SIZE_LEVEL - 1));
        parent = node;
        node = (struct radix_tree_node *)node->slots[offset];
        if (!node) {
            fit_search = 1;
            break;
        }
        shift -= BIT_LEVEL;
        height--;
    }

    if (fit_search) {
        node = parent;
        uint8_t temp = offset;
        //search for this level
        while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
            offset++;
            if (offset < SIZE_LEVEL) {
                node = radix_tree_node_left(node->slots[offset]);
                return node;
            }
        }
        //go back
        offset = temp;
        node = go_back_index(offset, node, index, shift);

        node = radix_tree_node_left(node);
    }
    if (!node) {
        return NULL;
    }

    return node;
}



void *radix_tree_delete(struct radix_tree_root *root, unsigned long index)
{
    struct radix_tree_node *node = root->rnode;
    uint8_t height = root->height;
    uint8_t shift = BIT_LEVEL * height;
    uint8_t offset;

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
    void *item = node->slots[offset];
    if (!item) {
        return NULL;
    }

    node->slots[offset] = NULL;
    node->count--;

    struct radix_tree_node *parent = NULL;
    while (node && (node->count == 0) && (node != root->rnode)) {
        offset = node->offset;
        parent = node->parent;
        heap_free(node);
        parent->count--;
        node = parent;
    }
    if (node) {
        node->slots[offset] = NULL;
    }

    if (node == root->rnode) {
        root->rnode = NULL;
        //It can replace by membitpool
        heap_free(node);
    }

    return item;
}

