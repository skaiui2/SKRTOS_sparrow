#include "radix.h"


__attribute__((always_inline)) inline uint8_t log2_clz64(uint64_t value)
{
    return 63 - __builtin_clzll(value);
}

static inline uint8_t radix_tree_height(uint64_t index)
{
    if (index == 0) return 1;

    uint8_t msb = log2_clz64(index);
    return msb / BIT_LEVEL + 1;
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
            .height = height
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
    uint8_t shift = (height - 1) * BIT_LEVEL;

    while (height-- > 1) {
        offset = (index >> shift) & (SIZE_LEVEL - 1);
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
        shift -= BIT_LEVEL;
    }

    struct radix_tree_node *leaf = node;
    offset = index & (SIZE_LEVEL - 1);
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
    int height = radix_tree_height(index);

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
 * The core idea of this implementation is to start from the most significant
 * bits of the index and consume BIT_LEVEL bits per tree level, expanding
 * downward until reaching the leaves. This allows the radix tree to scale
 * flexibly with different branching factors.
 *
 * Macros:
 *     #define BIT_LEVEL   N
 *     #define SIZE_LEVEL  (1 << BIT_LEVEL)
 *
 * - BIT_LEVEL defines how many bits are consumed at each level.
 * - SIZE_LEVEL defines the number of slots (children) per node.
 *
 * Height calculation:
 *     height = floor(msb / BIT_LEVEL) + 1
 * where msb is the index of the most significant bit. This ensures that
 * any remainder bits are correctly accounted for by adding one more level.
 *
 * Traversal principle:
 * - Initialize shift = (height - 1) * BIT_LEVEL
 * - At each level: offset = (index >> shift) & (SIZE_LEVEL - 1)
 * - Then shift -= BIT_LEVEL and descend to the next level
 * - At the leaf level, slots hold actual items, and rightward traversal
 *   supports upper-bound queries.
 *
 * By adjusting BIT_LEVEL, the tree can seamlessly switch between binary
 * (BIT_LEVEL=1), quaternary (BIT_LEVEL=2), or higher branching structures,
 * achieving scalable and flexible expansion.
 */

void *radix_tree_lookup(struct radix_tree_root *root, size_t index)
{
    struct radix_tree_node *node = root->rnode;
    int height = (int)root->height;
    uint32_t shift = (height - 1) * BIT_LEVEL;
    uint8_t offset;

    if (!node) {
        return NULL;
    }

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


void *radix_tree_node_left(struct radix_tree_node *node)
{
    int offset;
    int height;
    if (!node) {
        return NULL;
    }

    height = (int)node->height;
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

void *radix_tree_delete(struct radix_tree_root *root, size_t index)
{
    struct radix_tree_node *node = root->rnode;
    int height = (int)root->height;
    uint8_t shift = (height - 1) * BIT_LEVEL;
    uint8_t offset;

    if (height < radix_tree_height(index)) return NULL;

    while (height > 1) {
        if (!node) return NULL;
        offset = (index >> shift) & (SIZE_LEVEL - 1);
        node = (struct radix_tree_node *)node->slots[offset];
        shift -= BIT_LEVEL;
        height--;
    }

    if (!node) return NULL;

    offset = index & (SIZE_LEVEL - 1);
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
    uint8_t offset;
    uint32_t shift = (root->height - 1) * BIT_LEVEL;

    struct radix_tree_node *node = root->rnode;
    if (!node) return NULL;

    while (node && node->height > 1) {
        offset = (index >> shift) & (SIZE_LEVEL - 1);
        if (node->slots[offset]) {
            node = node->slots[offset];
            shift -= BIT_LEVEL;
        } else {
            while(++offset < SIZE_LEVEL) {
                if (node->slots[offset]) {
                    return radix_tree_node_left(node->slots[offset]);
                }
            }

            break;
        }
    }

    // if in leaf layer
    if (node && node->height == 1) {
        offset = index & (SIZE_LEVEL - 1);
        if (node->slots[offset]) {
            return node->slots[offset];
        }

        while (++offset < SIZE_LEVEL) {
            if (node->slots[offset]) {
                return node->slots[offset];
            }
        }
    }

    while (node && node->parent) {
        uint8_t off = node->offset;
        node = node->parent;
        while (++off < SIZE_LEVEL) {
            if (node->slots[off]) {
                return radix_tree_node_left(node->slots[off]);
            }
        }

    }

    return NULL;
}