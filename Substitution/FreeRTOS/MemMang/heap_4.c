#include<stdint.h>
#include<stdlib.h>

#define aligment_byte               0x07
#define config_heap   8*1024



#define Class(class)    \
typedef struct  class  class;\
struct class

#define MIN_size     ((size_t) (heapstructSize << 1))

Class(heap_node){
    heap_node *next;
    size_t blocksize;
};

Class(xheap){
    heap_node head;
    heap_node *tail;
    size_t allsize;
};

xheap theheap = {
        .tail = NULL,
        .allsize = config_heap,
};

static  uint8_t allheap[config_heap];
static const size_t heapstructSize = (sizeof(heap_node) + (size_t)(aligment_byte)) &~(aligment_byte);

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

void *pvPortMalloc(size_t wantsize)
{
    heap_node *prevnode;
    heap_node *usenode;
    heap_node *newnode;
    size_t aligmentrequisize;
    void *xReturn = NULL;
    wantsize += heapstructSize;
    if((wantsize & aligment_byte) != 0x00)
    {
        aligmentrequisize = (aligment_byte + 1) - (wantsize & aligment_byte);//must 8-byte alignment
        wantsize += aligmentrequisize;
    }//You can add the TaskSuspend function ,that make here be a atomic operation
    if(theheap.tail== NULL )
    {
        heap_init();
    }//Resume
    prevnode = &theheap.head;
    usenode = theheap.head.next;
    while((usenode->blocksize) < wantsize )
    {
        prevnode = usenode;
        usenode = usenode ->next;
        if(use_node == NULL){
            return xReturn;
        }
    }
    xReturn = (void*)( ( (uint8_t*)usenode ) + heapstructSize );
    prevnode->next = usenode->next ;
    if( (usenode->blocksize - wantsize) > MIN_size )
    {
        newnode = (void*)( ( (uint8_t*)usenode ) + wantsize );
        newnode->blocksize = usenode->blocksize - wantsize;
        usenode->blocksize = wantsize;
        newnode->next = prevnode->next ;
        prevnode->next = newnode;
    }
    theheap.allsize-= usenode->blocksize;
    usenode->next = NULL;
    return xReturn;
}

static void InsertFreeBlock(heap_node* xInsertBlock);
void vPortFree(void *xret)
{
    heap_node *xlink;
    uint8_t *xFree = (uint8_t*)xret;

    xFree -= heapstructSize;
    xlink = (void*)xFree;
    theheap.allsize += xlink->blocksize;
    InsertFreeBlock((heap_node*)xlink);
}

static void InsertFreeBlock(heap_node* xInsertBlock)
{
    heap_node *first_fitnode;
    uint8_t* getaddr;

    for(first_fitnode = &theheap.head;first_fitnode->next < xInsertBlock;first_fitnode = first_fitnode->next)
    { /*finding the fit node*/ }

    xInsertBlock->next = first_fitnode->next;
    first_fitnode->next = xInsertBlock;

    getaddr = (uint8_t*)xInsertBlock;
    if((getaddr + xInsertBlock->blocksize) == (uint8_t*)(xInsertBlock->next))
    {
        if(xInsertBlock->next != theheap.tail )
        {
            xInsertBlock->blocksize += xInsertBlock->next->blocksize;
            xInsertBlock->next = xInsertBlock->next->next;
        }
        else
        {
            xInsertBlock->next = theheap.tail;
        }
    }
    getaddr = (uint8_t*)first_fitnode;
    if((getaddr + first_fitnode->blocksize) == (uint8_t*) xInsertBlock)
    {
        first_fitnode->blocksize += xInsertBlock->blocksize;
        first_fitnode->next = xInsertBlock->next;
    }
}