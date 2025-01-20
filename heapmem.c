#include<stdint.h>
#include<stdlib.h>

#define alignment_byte               0x07
#define config_heap   8*1024



#define Class(class)    \
typedef struct  class  class;\
struct class

#define MIN_size     ((size_t) (HeapStructSize << 1))

Class(heap_node){
    heap_node *next;
    size_t BlockSize;
};

Class(xheap){
    heap_node head;
    heap_node *tail;
    size_t AllSize;
};

xheap TheHeap = {
        .tail = NULL,
        .AllSize = config_heap,
};

static  uint8_t AllHeap[config_heap];
static const size_t HeapStructSize = (sizeof(heap_node) + (size_t)(alignment_byte)) &~(alignment_byte);

void heap_init( void )
{
    heap_node *first_node;
    uint32_t start_heap ,end_heap;
    //get start address
    start_heap =(uint32_t) AllHeap;
    if( (start_heap & alignment_byte) != 0){
        start_heap += alignment_byte ;
        start_heap &= ~alignment_byte;
        TheHeap.AllSize -=  (size_t)(start_heap - (uint32_t)AllHeap);//byte alignment means move to high address,so sub it!
    }
    TheHeap.head.next = (heap_node *)start_heap;
    TheHeap.head.BlockSize = (size_t)0;
    end_heap = start_heap + (uint32_t)TheHeap.AllSize - (uint32_t)HeapStructSize;
    if( (end_heap & alignment_byte) != 0){
        end_heap &= ~alignment_byte;
        TheHeap.AllSize =  (size_t)(end_heap - start_heap );
    }
    TheHeap.tail = (heap_node *)end_heap;
    TheHeap.tail->BlockSize  = 0;
    TheHeap.tail->next =NULL;
    first_node = (heap_node *)start_heap;
    first_node->next = TheHeap.tail;
    first_node->BlockSize = TheHeap.AllSize;
}

void *heap_malloc(size_t wantsize)
{
    heap_node *prevnode;
    heap_node *use_node;
    heap_node *newnode;
    size_t aligmentrequisize;
    void *xReturn = NULL;
    wantsize += HeapStructSize;
    if((wantsize & alignment_byte) != 0x00)
    {
        aligmentrequisize = (alignment_byte + 1) - (wantsize & alignment_byte);//must 8-byte alignment
        wantsize += aligmentrequisize;
    }//You can add the TaskSuspend function ,that make here be a atomic operation
    if(TheHeap.tail== NULL )
    {
        heap_init();
    }//Resume
    prevnode = &TheHeap.head;
    use_node = TheHeap.head.next;
    while((use_node->BlockSize) < wantsize )
    {
        prevnode = use_node;
        use_node = use_node ->next;
        if(use_node == NULL){
            return xReturn;
        }
    }
    xReturn = (void*)( ( (uint8_t*)use_node ) + HeapStructSize );
    prevnode->next = use_node->next ;
    if( (use_node->BlockSize - wantsize) > MIN_size )
    {
        newnode = (void*)( ( (uint8_t*)use_node ) + wantsize );
        newnode->BlockSize = use_node->BlockSize - wantsize;
        use_node->BlockSize = wantsize;
        newnode->next = prevnode->next ;
        prevnode->next = newnode;
    }
    TheHeap.AllSize-= use_node->BlockSize;
    use_node->next = NULL;
    return xReturn;
}

static void InsertFreeBlock(heap_node* xInsertBlock);
void heap_free(void *xret)
{
    heap_node *xlink;
    uint8_t *xFree = (uint8_t*)xret;

    xFree -= HeapStructSize;
    xlink = (void*)xFree;
    TheHeap.AllSize += xlink->BlockSize;
    InsertFreeBlock((heap_node*)xlink);
}

static void InsertFreeBlock(heap_node* xInsertBlock)
{
    heap_node *first_fitnode;
    uint8_t* getaddr;

    for(first_fitnode = &TheHeap.head;first_fitnode->next < xInsertBlock;first_fitnode = first_fitnode->next)
    { /*finding the fit node*/ }

    xInsertBlock->next = first_fitnode->next;
    first_fitnode->next = xInsertBlock;

    getaddr = (uint8_t*)xInsertBlock;
    if((getaddr + xInsertBlock->BlockSize) == (uint8_t*)(xInsertBlock->next))
    {
        if(xInsertBlock->next != TheHeap.tail )
        {
            xInsertBlock->BlockSize += xInsertBlock->next->BlockSize;
            xInsertBlock->next = xInsertBlock->next->next;
        }
        else
        {
            xInsertBlock->next = TheHeap.tail;
        }
    }
    getaddr = (uint8_t*)first_fitnode;
    if((getaddr + first_fitnode->BlockSize) == (uint8_t*) xInsertBlock)
    {
        first_fitnode->BlockSize += xInsertBlock->BlockSize;
        first_fitnode->next = xInsertBlock->next;
    }
}
