#include "radix.h"


__attribute__( ( always_inline ) ) inline uint8_t log2_clz(uint32_t Table)
{
    return 31 - __builtin_clz(Table);
}


void radix_tree_init(struct radix_tree_root *root)
{
    *root = (struct radix_tree_root) {
            .count = 0,
            .height = 0,
            .rnode = NULL
    };
}

void radix_tree_node_init(struct radix_tree_node *node, uint8_t height)
{
    *node = (struct radix_tree_node) {
            .parent = NULL,
            .count = 0,
            .height = height,
            .slots[0] = NULL,
            .slots[1] = NULL
    };
}

struct radix_tree_node *radix_tree_node_alloc(struct radix_tree_root *root, uint8_t height)
{
    struct radix_tree_node *node = heap_malloc(sizeof(struct radix_tree_node));
    if (node == NULL) {
        return NULL;
    }

    root->count++;
    radix_tree_node_init(node, height);
    return node;
}


void radix_tree_node_free(struct radix_tree_root *root, struct radix_tree_node *node)
{
    heap_free(node);
    root->count--;
}

int radix_tree_grow_height(struct radix_tree_root *root, uint8_t height)
{
    while(root->height < height) {
        (root->height)++;
        struct radix_tree_node *new_node = radix_tree_node_alloc(root, root->height);
        if (!new_node) {
            return -1;
        }
        new_node->offset = 0;

        root->rnode->parent = new_node;
        new_node->slots[0] = root->rnode;
        new_node->count++;
        root->rnode = new_node;
    }
    return (int)root->height;
}

int radix_tree_grow_node(struct radix_tree_root *root, size_t index, void *item)
{
    struct radix_tree_node *node = root->rnode;
    uint8_t offset;
    uint8_t height = root->height;
    uint8_t shift;

    while (height-- > 1) {
        shift = height;
        offset = (index >> shift) & 1;
        if (!node->slots[offset]) {
            node->slots[offset] = radix_tree_node_alloc(root, height);
            if (!node->slots[offset]) {
                return -1;
            }

            node->count++;
            ((struct radix_tree_node *)node->slots[offset])->offset = offset;
            ((struct radix_tree_node *)node->slots[offset])->parent = node;
        }
        node = (struct radix_tree_node *)node->slots[offset];
    }

    struct radix_tree_node *leaf = node;
    offset = index & 1;
    if (leaf->slots[offset]) {
        return -1;
    }
    leaf->slots[offset] = item;
    leaf->count++;

    return 0;
}


int radix_tree_insert(struct radix_tree_root *root, size_t index, void *item)
{
    struct radix_tree_node **node_ptr = &root->rnode;
    uint8_t height = log2_clz(index) + 1;

    //If no node, it can grow node no need to grow height!
    if (!*node_ptr) {
        *node_ptr = radix_tree_node_alloc(root, height);
        if (!*node_ptr) {
            return -1;
        }
        (*node_ptr)->parent = (struct radix_tree_node *)root;
        root->height = height;
        return radix_tree_grow_node(root, index, item);
    }

    if (height > root->height) {
        height = radix_tree_grow_height(root, height);
        if (height < 0) {
            return height;
        }
    }

    return radix_tree_grow_node(root, index, item);
}

/*
 * Note:
 * The variable `shift` is not hard‑wired to `height`.
 * By default, shift = height - 1, but if you want to change
 * the branching factor (e.g. implement path compression or
 * use a different number of bits per level), you can redefine
 * it with your own macro, such as:
 *
 *     shift = BIT_LEVEL * height;
 *
 * In other words, users are free to customize how many index
 * bits are consumed per tree level by adjusting this definition.
 */

void *radix_tree_lookup(struct radix_tree_root *root, size_t index)
{
    struct radix_tree_node *node = root->rnode;
    uint8_t height = root->height;
    int shift = height - 1;
    uint8_t offset;

    if (!node) {
        return NULL;
    }

    while (height > 0) {
        offset = (int)((index >> shift) & 1);
        node = (struct radix_tree_node *)node->slots[offset];
        if (!node) {
            return NULL;
        }
        shift--;
        height--;
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
        while ((offset < 2) && (!node->slots[offset])) {
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

void *radix_tree_delete(struct radix_tree_root *root, unsigned long index)
{
    struct radix_tree_node *node = root->rnode;
    uint8_t height = root->height;
    uint8_t shift = height - 1;
    uint8_t offset;

    if (height < log2_clz(index)) return NULL;

    while (height > 1) {
        if (!node) return NULL;
        offset = (index >> shift) & 1;
        node = (struct radix_tree_node *)node->slots[offset];
        shift--;
        height--;
    }

    if (!node) return NULL;

    offset = index & 1;
    void *item = node->slots[offset];
    if (!item) return NULL;

    node->slots[offset] = NULL;
    node->count--;

    while (node->count == 0 && node != root->rnode) {
        struct radix_tree_node *parent = node->parent;
        uint8_t off = node->offset;
        radix_tree_node_free(root, node);
        parent->slots[off] = NULL;
        parent->count--;
        node = parent;
    }

    if (root->rnode && root->rnode->count == 0) {
        radix_tree_node_free(root, node);
        root->rnode = NULL;
        root->height = 0;
    }

    return item;
}

void *radix_tree_lookup_upper_bound(struct radix_tree_root *root, size_t index) {
    struct radix_tree_node *node = root->rnode;
    if (!node) return NULL;

    unsigned int shift = root->height - 1;
    uint8_t offset;

    while (node && node->height > 1) {
        offset = (index >> shift) & 1;
        if (node->slots[offset]) {
            node = node->slots[offset];
            shift--;
        } else {
            if (offset + 1 < 2 && node->slots[offset+1]) {
                return radix_tree_node_left(node->slots[offset+1]);
            }
            break;
        }
    }

    // if in leaf layer
    if (node && node->height == 1) {
        offset = index & 1;
        if (node->slots[offset]) {
            return node->slots[offset];
        }
        if (offset + 1 < 2 && node->slots[offset+1]) {
            return node->slots[offset+1];
        }
    }

    while (node && node->parent) {
        int off = node->offset;
        node = node->parent;
        if (off + 1 < 2 && node->slots[off+1]) {
            return radix_tree_node_left(node->slots[off+1]);
        }
    }

    return NULL;
}