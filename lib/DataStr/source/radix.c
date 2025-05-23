#include "radix.h"


void radix_tree_init(struct radix_tree_root *root)
{
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

struct radix_tree_node *radix_tree_node_alloc(uint8_t height)
{
    struct radix_tree_node *node = heap_malloc(sizeof(struct radix_tree_node));
    if (node == NULL) {
        return NULL;
    }
    radix_tree_node_init(node, height);

    return node;
}



int radix_tree_insert(struct radix_tree_root *root, size_t index, void *item)
{
    struct radix_tree_node **node_ptr = &root->rnode;
    uint8_t height = root->height;
    uint8_t shift;
    uint8_t offset;

    if (!*node_ptr) {
        root->rnode = radix_tree_node_alloc(0);
        root->rnode->parent = (struct radix_tree_node *)root;
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
        new_node->parent = root->rnode->parent;
        root->rnode->parent = new_node;
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
    leaf->count++;

    if (root->max_size < index) {
        root->max_size = index;
    }

    return 0;
}


void *radix_tree_lookup(struct radix_tree_root *root, size_t index)
{
    struct radix_tree_node *node = root->rnode;
    uint8_t height = root->height;
    uint8_t shift = BIT_LEVEL * height;
    uint8_t offset;

    if (index > root->max_size) {
        return NULL;
    }

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

/*
 * The last bit can cause issues because we do not perform a backtracking operation at that stage.
 * So, we must ensure that the index is always aligned to at least 2.
 */
void *radix_tree_lookup_upper_bound(struct radix_tree_root *root, size_t index) {
    struct radix_tree_node *node = root->rnode;
    uint8_t height = root->height;
    uint8_t shift = BIT_LEVEL * height;
    uint8_t offset;
    uint8_t first_search = 1;

    if (index > root->max_size) {
        return NULL;
    }

    while (height > 0) {
        if (first_search) {
            offset = (index >> shift) & (SIZE_LEVEL - 1);
        } else {
            offset = 0;
        }

        //search for this level
        while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
            offset++;
            if (first_search) {
                first_search = 0;
            }
        }

        //go back
        while ((offset >= SIZE_LEVEL) || (!node->slots[offset])) {
            node = node->parent;
            shift += BIT_LEVEL;
            height++;
            offset = ((index >> shift) & (SIZE_LEVEL - 1)) + 1;
            while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
                offset++;
            }
            if ((offset < SIZE_LEVEL) && node->slots[offset]) {
                node = node->slots[offset];
                shift -= BIT_LEVEL;
                height--;
                break;
            }

            if (first_search) {
                first_search = 0;
            }
        }
        if (first_search == 0) {
            offset = 0;
            while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
                offset++;
            }
        }

        node = (struct radix_tree_node *)node->slots[offset];
        shift -= BIT_LEVEL;
        height--;
    }

    if (first_search) {
        offset = (int) (index & (SIZE_LEVEL - 1));
    } else {
        offset = 0;
        while ((offset < SIZE_LEVEL) && (!node->slots[offset])) {
            offset++;
        }
    }
    if (node->slots[offset]) {
        return node->slots[offset];
    }
    return NULL;
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
    if (!node->slots[offset]) {
        return NULL;
    }

    node->slots[offset] = NULL;
    node->count--;

    return node->slots[offset];
}

