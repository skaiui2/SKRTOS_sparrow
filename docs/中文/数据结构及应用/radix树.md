## radix树

在TCP/IP中，radix树可以被用来作为设计路由表的数据结构。而在Linux内核中，也被应用于内存管理相关的算法。



## radix树原理与实现

作为字典树的变种，radix由于本身的动态特性，即使概念并不复杂，但考虑性能，其实真正的实现是比较复杂的。

### 数据结构

```

struct radix_tree_node {
    struct radix_tree_node *parent;
    unsigned int height;
    unsigned int count;
    unsigned char offset;
    void *slots[SIZE_LEVEL];
};

struct radix_tree_root {
    size_t max_size;
    unsigned int height;
    struct radix_tree_node *rnode;
    unsigned int count;
};

```

节点高度的作用是对位的映射。

当SIZE_LEVEL等于2时，对于每一颗树而言，有：

节点挂满的状态，此时趋向于一颗完全二叉树：

```

        [Root] (height=2)
           |
   		---------------------------
   		/                	 	\
 	[Node] (h=2)     		 	[Node] (h=1)
 	  /           \              /		     \
	[N]   		 [N]       		[N]           [N]    (h=0)
   /    \      /     \         /    \    	 /    \
 [0]    [1]  [2]    [3]       [4]    [5]    [6]    [7]  (叶子节点)

```

如果我们需要存储2^3节点，那么需要消耗3位.





### 初始化

```

void radix_tree_init(struct radix_tree_root *root)
{
    root->height = 0;
    root->rnode = NULL;
}

```

使用动态内存分配：

heap_malloc作为内核最简单易用的内存管理实现，是所有内存管理方法的子集，但是，由于heap_mallc的复杂度为O(N)(链表策略)或O(logN)（红黑树策略），且radix树节点本身的大小就是固定的，其树的大小的动态范围也是固定的，所以，这里应该由另一种内存池O(1)算法替代，不过目前还未更改。

```

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

```

### 插入操作

```

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

```

1.先判断树是否为空，如果是，那就直接生长，如果不是，就调整树高，由于需要使用位图进行映射，考虑操作效率，选择树的增长为自顶向上生长，而非从根节点向下生长。因此我们使用的radix树需要判断位图是否可容纳当前树的叶子节点。

2.对于每一高度的树，节点都对应位图上BIT_LEVEL的占用个数，因此在拓展树高后，需要拓展并补充树的节点，如果仍然没有生长到想要的位置，那就继续扩展，如果已经生长到需要的位置，那么就停止。

3.树生长停止时，此时恰好位于该叶子节点上，因此结构体中的子树指针可被复用为指向映射数据。



### 遍历操作

```

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

```

根据位图查找即可，就不多赘述了。

### 上界查找

```

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


```

1.分为正常映射查找和上界查找

2.当正常查找获取空指针时，进行回溯操作，向上、向右对节点进行迭代，直到找到一个在先前节点右边的节点

3.此时舍弃原先的位图遍历，从0逐个开始查找，获取到的节点不为NULL即可，再对每一层重复操作

4.迭代到最后一层，获取叶子节点并返回

### 最小节点查找

```

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
```



### 删除操作

```


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







