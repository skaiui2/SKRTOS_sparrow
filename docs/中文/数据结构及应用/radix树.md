## radix树

在TCP/IP中，radix树可以被用来作为设计路由表的数据结构。而在Linux内核中，也被应用于内存管理相关的算法。



## 内存管理算法设计

内核中许多对象及结构体，大小都是固定的，使用动态且随机开辟内存的情况是比较少的。

而且，结构体如果严格按照主流CPU架构来设计，那么必须对大小及存放位置有所要求。结构体过小，cache命中率可能会降低，甚至出现大量断页，结构体放置的位置没有字节对齐，那么会降低访问速度。虽然目前内核并不支持虚拟内存，但是程序也应当具有良好的空间局部性。



基于此，算法思路如下：

1.使用radix进行位映射，连续位的大小映射可用内存的大小。

2.申请内存时，直接根据radix树遍历即可。

3.可用内存块与申请内存块大小差距过大，参考buddy算法进行二分即可。

4.需要使用连接链表维护各个内存块地址相对大小，也应当在结构体中添加字段进行染色，区分局部内存池可用、不可用等情况。

5.确保内存块必须为2的一定指数大小。

这其实类似于内存池机制，但是需要进行动态切割及合并。



## radix树原理与实现

作为字典树的变种，radix由于本身的动态特性，即使概念并不复杂，但考虑性能，其实真正的实现是比较复杂的。

### 数据结构

```

struct radix_tree_node {
    unsigned int height;
    unsigned int count;
    void *slots[SIZE_LEVEL];
};

struct radix_tree_root {
    unsigned int height;
    struct radix_tree_node *rnode;
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

heap_malloc作为内核最简单易用的内存管理实现，是所有内存管理方法的子集

```
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
```

### 插入操作

```

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

```

1.调整树高，由于需要使用位图进行映射，考虑操作效率，选择树的增长为自顶向上生长，而非从根节点向下生长。因此我们使用的radix树需要判断位图是否可容纳当前树的叶子节点。

2.对于每一高度的树，节点都对应位图上BIT_LEVEL的占用个数，因此在拓展树高后，需要拓展并补充树的节点，如果仍然没有生长到想要的位置，那就继续扩展，如果已经生长到需要的位置，那么就停止。

3.树生长停止时，此时恰好位于该叶子节点上，因此结构体中的子树指针可被复用为指向映射数据。



### 遍历操作

```


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

```

根据位图查找即可，就不多赘述了。



### 删除操作

```

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

```

1.与遍历操作同理，根据位图查找即可

2.查找到后，初始化相关指针并使用heap_free回收相关内存





