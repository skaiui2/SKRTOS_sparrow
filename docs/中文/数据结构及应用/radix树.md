## radix树

在TCP/IP中，radix树可以被用来作为设计路由表的数据结构。而在Linux内核中，也被应用于内存管理相关的算法。

当然，Linux内核中是被用来作虚拟内存和物理内存的映射，而在这里，被用来实现类tlsf内存管理算法。

我们这里实现的是映射查找radix树，与路由匹配的radix树并不是同一种。



## radix树原理与实现

作为字典树的变种，radix由于本身的动态特性，其实真正的实现是比较复杂的。

我们实现的radix树的功能：基本的映射功能、分支伸缩、动态调整、支持最小节点查找、支持上界查找。

### 数据结构

```c

#define BIT_LEVEL   4
#define SIZE_LEVEL  (1 << BIT_LEVEL)


struct radix_tree_node {
    void *slots[SIZE_LEVEL];
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
```

节点高度的作用是对位的映射。

对于每一颗树而言，有：

节点挂满的状态，此时趋向于一颗完全二叉树：

```

        [Root] (height=3)
           |
          [rn_node] (height = 3)
   		---------------------------
   		/                	 	\
 	[Node] (h=2)     		 	[Node] (h=2)
 	  /           \              /		     \
	[N]   		 [N]       		[N]           [N]    (h=1)
   /    \      /     \         /    \    	 /    \
 [0]    [1]  [2]    [3]       [4]    [5]    [6]    [7]  (叶子节点)

```

如果我们需要存储2^3节点，那么需要消耗3位.



### 计算总bit数

```c

__attribute__((always_inline)) inline uint8_t log2_clz64(uint64_t value)
{
    return 63 - __builtin_clzll(value);
}
```

这被用来计算高度。

### 高度计算

对于一个数来说，如何计算对应的radix树的高度呢？

比如二进制111，如果2个bit为一层，实际上这棵树有2层，计算过程是：3/2 + 1，但是对于二进制11来说，计算过程是2/2，不用加一。

诚然，我们可以通过判断一个数是否有余数来决定是否加1，但是这样效率有点低。

我们可以用下面的方法计算，其中msb等于总bit数 - 1.

```c
static inline uint8_t radix_tree_height(uint64_t index)
{
    if (index == 0) return 1;

    uint8_t msb = log2_clz64(index);
    return msb / BIT_LEVEL + 1;
}
```

为了严谨，让我们证明一下：

#### 证明

- 设定：msb = qk + r，其中 q = msb / k (取整)，0 ≤ r < k
- 括号 [ ]：表示有余数自动加 1，即上取整

### Height 推导

- h = [(msb + 1) / k]
- 代入 msb = qk + r ，其中0 <= r < k，得： h = [(qk + r + 1) / k] = q + [(r + 1) / k]
- 因为 1 ≤ r+1 ≤ k，所以 [(r+1)/k] = 1
- 因此 h = q + 1 = (msb / k) + 1 (取整)



### 初始化

```c

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
```

使用动态内存分配：

heap_malloc作为内核最简单易用的内存管理实现，是所有内存管理方法的子集，但是，由于heap_mallc的复杂度为O(N)(链表策略)或O(logN)（红黑树策略），且radix树节点本身的大小就是固定的，其树的大小的动态范围也是固定的，所以，这里应该由另一种内存池O(1)算法替代，不过目前还未更改。

```c

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

```

### 插入操作

```c

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
```

1.先判断树是否为空，如果是，那就直接生长，如果不是，就调整树高，由于需要使用位图进行映射，考虑操作效率，选择树的增长为自顶向上生长，而非从根节点向下生长。因此我们使用的radix树需要判断位图是否可容纳当前树的叶子节点。

2.对于每一高度的树，节点都对应位图上bit的占用个数，因此在拓展树高后，需要拓展并补充树的节点，如果仍然没有生长到想要的位置，那就继续扩展，如果已经生长到需要的位置，那么就停止。

3.树生长停止时，此时恰好位于该叶子节点上，因此结构体中的子树指针可被复用为指向映射数据。



### 遍历操作

```c

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
```

根据位图查找即可，就不多赘述了。

### 上界查找

```c

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
```

1.分为正常映射查找和上界查找

2.当正常查找获取空指针时，进行回溯操作，向上、向右对节点进行迭代，直到找到一个在先前节点右边的节点

3.此时舍弃原先的位图遍历，从0逐个开始查找，获取到的节点不为NULL即可，再对每一层重复操作

4.迭代到最后一层，获取叶子节点并返回

### 最小节点查找

```c
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
```



### 删除操作

```c
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
```

1.与遍历操作同理，根据位图查找即可

2.查找到后，初始化相关指针即可，然后自底向上，逐个回收空闲的radix树节点。



## 内存管理算法设计

内核中许多对象及结构体，大小都是固定的，使用动态且随机开辟内存的情况是比较少的。

而且，结构体如果严格按照主流CPU架构来设计，那么必须对大小及存放位置有所要求。结构体过小，cache命中率可能会降低，甚至出现大量断页，结构体放置的位置没有字节对齐，那么会降低访问速度。虽然目前内核并不支持虚拟内存，但是程序也应当具有良好的空间局部性。



这其实类似于内存池机制，但是需要进行动态切割及合并。

基于此，算法思路如下：

1.使用radix进行位映射，连续位的大小映射可用内存的大小。

2.申请内存时，根据radix树遍历，此时出现两种情况：位图映射命中和不命中。对于前者，直接返回该内存节点即可。

3.不命中时，需要在radix树上向高位进行回溯，由于先前的路径是错误的，所以需要在原先的位上向右移动，直到找到一个存在的节点，那么该节点的左子树的最小节点就是满足我们需求的best-fit节点：在整颗radix树上，这是第一个找到的，但也是刚好大于需求大小的节点。

4.找到该节点后，如果可用内存块与申请内存块大小差距过大，需进行切割。

5.我们使用链表地址映射该节点，因此我们通过该地址找到链表，并获取链表第一个节点（由于对应位图都相同，因此无需区分大小）

6.使用连接链表维护各个内存块地址相对大小，方便合并操作。

## 问题

### radix树的拓展

在设计radix树时，有一个问题需要解决，那就是在插入节点时，如果当前插入index大于当前树的高度，必须要拓展radix树。那么我们面临一个问题：如何根据index进行拓展？

### 根据index动态拓展

当传入较高的index时，我们必须检查现有树的高度以及存在的节点，比较理想的思路是：先保存原先的根节点，再根据新的index进行拓展，最后将原先保存的根节点挂载道对应路径上。

radix树设计参考的是linux内核的方法，从低位向高位拓展，高位为0则忽略。换而言之，树的结构是向上增长，root是树顶。这与radix的遍历思路截然相反，在拓展时，我们并没有采用自顶向下生长的方式。

根据index的高度，对最左边的节点进行拓展，由于之前的根节点代表0，所以将其更改为新增节点的0下标位置。对于该节点，上面的位都是0，因此我们需要计算高度，然后向上每一层拓展0下标的节点。

radix的拓展是从低位向高位拓展的，这更符合我们的习惯，往往我们设置index时，都习惯于先使用小数字。

在拓展后，原先根节点节点上面的节点已经支撑了整体的高度，所以我们现在根据index的位图，自上而下不断新增加节点即可。

因此，策略如下：

1.如果树为空，直接生长即可

2.树不为空，不断在root处新增节点，直到满足高度要求，然后让树直接生长

因此，radix_tree_insert的伪代码如下：

```

int radix_tree_insert()
{

    if (树为空) {
        先申请rn_node节点
        由于没有节点，树生长;
        返回
    }

    while (树的高度太低了) {
        申请节点
        更改父子节点关系
        新节点继承之前的节点的信息，如指针指向、高度等
        树高加1
    }
	高度已经满足，树生长
	返回
}
```





## 测试

目前radix树是可以正常使用的，在之前的一段时间，上界查找有一些问题。笔者花了一下午时间，修复好了程序。

测试程序如下，其中上界查找是重点，程序已全部通过：

这里使用队列是为了进行层序遍历，方便打印调试。

```c
// 队列节点
typedef struct queue_item {
    struct radix_tree_node *node;
    int level;
    struct queue_item *next;
} queue_item_t;

typedef struct {
    queue_item_t *front, *rear;
} queue_t;

static void enqueue(queue_t *q, struct radix_tree_node *node, int level) {
    queue_item_t *n = malloc(sizeof(queue_item_t));
    n->node = node;
    n->level = level;
    n->next = NULL;
    if (!q->rear) q->front = q->rear = n;
    else { q->rear->next = n; q->rear = n; }
}

static queue_item_t* dequeue(queue_t *q) {
    if (!q->front) return NULL;
    queue_item_t *n = q->front;
    q->front = n->next;
    if (!q->front) q->rear = NULL;
    return n;
}

// 层序遍历打印，可伸缩版本
void print_radix_tree_bfs(const struct radix_tree_root *root) {
    if (!root || !root->rnode) {
        printf("Empty tree\n");
        return;
    }

    printf("RadixTree Root (height=%u)\n", root->height);

    queue_t q = {0};
    enqueue(&q, root->rnode, 0);

    int current_level = -1;

    while (q.front) {
        queue_item_t *qi = dequeue(&q);
        struct radix_tree_node *node = qi->node;
        int level = qi->level;

        if (level != current_level) {
            current_level = level;
            printf("\n[Level %d] ", level);
        }

        printf("[Node@%p h=%u] ", (void*)node, node->height);

        if (node->height > 1) {
            // 内部节点：slots 指向子节点
            for (int i = 0; i < SIZE_LEVEL; i++) {
                void *p = node->slots[i];
                if (p) {
                    struct radix_tree_node *child = (struct radix_tree_node*)p;
                    printf("slot[%d]=Node@%p(h=%u, count=%u) ",
                           i, (void*)child, child->height, child->count);
                    enqueue(&q, child, level + 1);
                } else {
                    printf("slot[%d]=NULL ", i);
                }
            }
        } else if (node->height == 1) {
            // 叶子节点：slots 存放的是 int* 值
            printf("node's h[%d], count(%d) ", node->height, node->count);
            for (int i = 0; i < SIZE_LEVEL; i++) {
                int *val = (int*)node->slots[i];
                if (val) {
                    printf("slot[%d]=Value(%d) ", i, *val);
                } else {
                    printf("slot[%d]=NULL ", i);
                }
            }
        }

        free(qi);
    }
    printf("\n");
}



#include <assert.h>
#include <stdio.h>

void test_upper_bound_small() {
    struct radix_tree_root tree;
    radix_tree_init(&tree);

    int v10 = 10, v20 = 20, v40 = 40, v100 = 100;

    // 插入几个稀疏键
    assert(radix_tree_insert(&tree, 10, &v10) == 0);
    assert(radix_tree_insert(&tree, 20, &v20) == 0);
    assert(radix_tree_insert(&tree, 40, &v40) == 0);
    assert(radix_tree_insert(&tree, 100, &v100) == 0);

    // 精确命中
    int *r = (int*)radix_tree_lookup_upper_bound(&tree, 10);
    assert(r && *r == 10);

    r = (int*)radix_tree_lookup_upper_bound(&tree, 20);
    assert(r && *r == 20);

    // 落在空洞之间
    r = (int*)radix_tree_lookup_upper_bound(&tree, 15);
    assert(r && *r == 20);

    r = (int*)radix_tree_lookup_upper_bound(&tree, 25);
    assert(r && *r == 40);

    r = (int*)radix_tree_lookup_upper_bound(&tree, 41);
    assert(r && *r == 100);

    // 超过最大键
    r = (int*)radix_tree_lookup_upper_bound(&tree, 101);
    assert(r == NULL);

    // 删除一个键后再测试
    int *d = (int*)radix_tree_delete(&tree, 20);
    assert(d && *d == 20);

    print_radix_tree_bfs(&tree);

    r = (int*)radix_tree_lookup_upper_bound(&tree, 15);

    assert(r && *r == 40); // 20 已删除，应该跳到 40

    printf("Upper bound small test passed.\n");
}

#include <assert.h>
#include <stdlib.h>
#include <time.h>

void test_upper_bound_medium() {
    struct radix_tree_root tree;
    radix_tree_init(&tree);

    // 插入稀疏键
    int v10=10, v20=20, v40=40, v60=60, v100=100;
    radix_tree_insert(&tree, 10, &v10);
    radix_tree_insert(&tree, 20, &v20);
    radix_tree_insert(&tree, 40, &v40);
    radix_tree_insert(&tree, 60, &v60);
    radix_tree_insert(&tree, 100, &v100);

    // 精确命中
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 10) == 10);
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 20) == 20);

    // 空洞跳转
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 15) == 20);
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 25) == 40);
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 41) == 60);

    // 超过最大键
    assert(radix_tree_lookup_upper_bound(&tree, 101) == NULL);

    // 删除一个中间键
    radix_tree_delete(&tree, 40);
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 25) == 60);

    // 删除最小键
    radix_tree_delete(&tree, 10);
    assert(*(int*)radix_tree_lookup_upper_bound(&tree, 5) == 20);

    // 删除最大键
    radix_tree_delete(&tree, 100);
    assert(radix_tree_lookup_upper_bound(&tree, 90) == NULL);
}

void test_upper_bound_random() {
    struct radix_tree_root tree;
    radix_tree_init(&tree);

    const int N = 5000;
    int *vals = malloc(N * sizeof(int));
    int present[N];
    for (int i = 0; i < N; i++) present[i] = 0;

    srand(12345);

    // 随机插入一半的键
    for (int i = 0; i < N; i++) {
        if (rand() % 2) {
            vals[i] = i;
            radix_tree_insert(&tree, i, &vals[i]);
            present[i] = 1;
        }
    }

    // 随机查询上界
    for (int q = 0; q < 5000; q++) {
        int idx = rand() % N;
        int expected = -1;
        for (int k = idx; k < N; k++) {
            if (present[k]) { expected = k; break; }
        }
        int *res = (int*)radix_tree_lookup_upper_bound(&tree, idx);
        if (expected == -1) {
            assert(res == NULL);
        } else {
            assert(res && *res == expected);
        }
    }

    // 随机删除并再次验证
    for (int i = 0; i < N; i++) {
        if (present[i] && (rand() % 3 == 0)) {
            radix_tree_delete(&tree, i);
            present[i] = 0;
        }
    }

    for (int q = 0; q < 2000; q++) {
        int idx = rand() % N;
        int expected = -1;
        for (int k = idx; k < N; k++) {
            if (present[k]) { expected = k; break; }
        }
        int *res = (int*)radix_tree_lookup_upper_bound(&tree, idx);
        if (expected == -1) {
            assert(res == NULL);
        } else {
            assert(res && *res == expected);
        }
    }

    free(vals);
}

void run_upper_bound_tests() {
    test_upper_bound_small();   // 之前的小规模测试
    test_upper_bound_medium();  // 系统性覆盖
    test_upper_bound_random();  // 随机压力测试
}
#include <assert.h>
#include <stdlib.h>

void run_radix_tree_tests() {
    struct radix_tree_root tree;
    radix_tree_init(&tree);

    // 插入数据
    int values[10];
    for (int i = 0; i < 10; i++) {
        values[i] = i * 100;
        int ret = radix_tree_insert(&tree, i, &values[i]);
        assert(ret == 0);
    }

    // 查找测试
    for (int i = 0; i < 10; i++) {
        int *res = (int*)radix_tree_lookup(&tree, i);
        assert(res && *res == values[i]);
    }

    // 删除部分节点（偶数索引）
    for (int i = 0; i < 10; i += 2) {
        int *res = (int*)radix_tree_delete(&tree, i);
        assert(res && *res == values[i]);
    }

    // 再次查找，确认删除效果
    for (int i = 0; i < 10; i++) {
        int *res = (int*)radix_tree_lookup(&tree, i);
        if (i % 2 == 0) {
            assert(res == NULL); // 偶数已删除
        } else {
            assert(res && *res == values[i]); // 奇数仍存在
        }
    }

    // 删除不存在的 index
    int *res = (int*)radix_tree_delete(&tree, 99);
    assert(res == NULL);

    // 重复删除同一个 index
    res = (int*)radix_tree_delete(&tree, 1);
    assert(res && *res == values[1]);
    res = (int*)radix_tree_delete(&tree, 1);
    assert(res == NULL);

    // 最终清空
    for (int i = 0; i < 10; i++) {
        (void)radix_tree_delete(&tree, i);
    }
    assert(tree.count == 0);
    assert(tree.rnode == NULL);
}

int main() {
    run_radix_tree_tests();
    run_upper_bound_tests();
    return 0;
}
```



