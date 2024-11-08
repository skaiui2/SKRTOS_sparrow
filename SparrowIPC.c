#include "core_cm3.h"
#include<stdint.h>
#include<stdlib.h>

#define true    1
#define false   0

#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )

#define aligment_byte               0x07
#define config_heap   8*1024
#define configMaxPriori 32
#define configShieldInterPriority 191

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



void heap_init()
{
    heap_node *firstnode;
    uint32_t start_heap ,end_heap;
    //get start address
    start_heap =(uint32_t) allheap;
    if( (start_heap & aligment_byte) != 0){
        start_heap += aligment_byte ;
        start_heap &= ~aligment_byte;
        theheap.allsize -=  (size_t)(start_heap - (uint32_t)allheap);//byte aligment means move to high address,so sub it!
    }//Init head node
    theheap.head.next = (heap_node *)start_heap;
    theheap.head.blocksize = (size_t)0;
    end_heap  = ( start_heap + (uint32_t)config_heap);
    if( (end_heap & aligment_byte) != 0){
        end_heap += aligment_byte ;
        end_heap &= ~aligment_byte;
        theheap.allsize =  (size_t)(end_heap - start_heap );
    }//Init tail node
    theheap.tail = (heap_node*)end_heap;
    theheap.tail->blocksize  = 0;
    theheap.tail->next =NULL;
    firstnode = (heap_node*)start_heap;
    firstnode->next = theheap.tail;
    firstnode->blocksize = theheap.allsize;
}

void *heap_malloc(size_t wantsize)
{
    heap_node *prevnode;
    heap_node *usenode;
    heap_node *newnode;
    size_t aligmentrequisize;
    void *xReturn = NULL;
    wantsize += heapstructSize;
    if((wantsize & aligment_byte) != 0x00) {
        aligmentrequisize = (aligment_byte + 1) - (wantsize & aligment_byte);//must 8-byte alignment
        wantsize += aligmentrequisize;
    }//You can add the TaskSuspend function ,that make here be a atomic operation
    if(theheap.tail== NULL ) {
        heap_init();
    }//Resume
    prevnode = &theheap.head;
    usenode = theheap.head.next;
    while((usenode->blocksize) < wantsize ) {//check the size is fit
        prevnode = usenode;
        usenode = usenode->next;
    }
    xReturn = (void*)( ( (uint8_t*)usenode ) + heapstructSize );
    prevnode->next = usenode->next ;
    if( (usenode->blocksize - wantsize) > MIN_size ) {//cut block
        newnode = (void *) (((uint8_t *) usenode) + wantsize);
        newnode->blocksize = usenode->blocksize - wantsize;
        usenode->blocksize = wantsize;
        newnode->next = prevnode->next;
        prevnode->next = newnode;
    }//Finish cutting
    theheap.allsize-= usenode->blocksize;
    usenode->next = NULL;
    return xReturn;
}

static void InsertFreeBlock(heap_node* xInsertBlock);
void heap_free(void *xret)
{
    heap_node *xlink;
    uint8_t *xFree = (uint8_t*)xret;

    xFree -= heapstructSize;//get the start address of the heap struct
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
    first_fitnode->next = xInsertBlock;//Insert Block

    getaddr = (uint8_t*)xInsertBlock;//Check whether the addresses are adjacent,then decide whether to merge 
    if((getaddr + xInsertBlock->blocksize) == (uint8_t*)(xInsertBlock->next)) {
        if (xInsertBlock->next != theheap.tail) {
            xInsertBlock->blocksize += xInsertBlock->next->blocksize;
            xInsertBlock->next = xInsertBlock->next->next;
        } else {
            xInsertBlock->next = theheap.tail;
        }
    }
    getaddr = (uint8_t*)first_fitnode;
    if((getaddr + first_fitnode->blocksize) == (uint8_t*) xInsertBlock) {
        first_fitnode->blocksize += xInsertBlock->blocksize;
        first_fitnode->next = xInsertBlock->next;
    }
}


Class(Stack_register)
{
    //automatic stacking
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    //manual stacking
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t LR;
    uint32_t PC;
    uint32_t xPSR;
};

Class(TCB_t)
{
    volatile uint32_t * pxTopOfStack;
    unsigned long uxPriority;
    uint32_t * pxStack;
    Stack_register *self_stack;//Save the status of the stack in the task !You can use gdb to debug it!
};
typedef  TCB_t         *TaskHandle_t;

__attribute__( ( used ) )  TCB_t * volatile pxCurrentTCB = NULL;
typedef void (* TaskFunction_t)( void * );

#define switchTask()\
*( ( volatile uint32_t * ) 0xe000ed04 ) = 1UL << 28UL;


uint32_t  NextTicks = ~(uint32_t)0;
uint32_t  TicksBase = 0;

uint32_t Ready = 0;
TaskHandle_t TcbTaskTable[configMaxPriori];

uint32_t Delay = 0;
uint32_t TicksTable[configMaxPriori];
uint32_t TicksTableAssist[configMaxPriori];
uint32_t* WakeTicksTable;
uint32_t* OverWakeTicksTable;
#define TicksTableInit( ) {  \
    WakeTicksTable = TicksTable;    \
    OverWakeTicksTable = TicksTableAssist;  \
}
#define TicksTableSwitch( ){    \
    uint32_t *temp;             \
    temp = WakeTicksTable;      \
    WakeTicksTable = OverWakeTicksTable; \
    OverWakeTicksTable = temp;  \
    }

uint32_t Suspend = 0;
//the table is defined for signal mechanism
uint32_t Block = 0;


#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

void __attribute__( ( naked ) )  vPortSVCHandler( void )
{
    __asm volatile (
            "	ldr	r3, pxCurrentTCBConst2		\n"
            "	ldr r1, [r3]					\n"
            "	ldr r0, [r1]					\n"
            "	ldmia r0!, {r4-r11}				\n"
            "	msr psp, r0						\n"
            "	isb								\n"
            "	mov r0, #0 						\n"
            "	msr	basepri, r0					\n"
            "	orr r14, #0xd					\n"
            "	bx r14							\n"
            "									\n"
            "	.align 4						\n"
            "pxCurrentTCBConst2: .word pxCurrentTCB				\n"
            );
}

void __attribute__( ( naked ) )  xPortPendSVHandler( void )
{
    __asm volatile
            (
            "	mrs r0, psp							\n"
            "	isb									\n"
            "										\n"
            "	ldr	r3, pxCurrentTCBConst			\n"
            "	ldr	r2, [r3]						\n"
            "										\n"
            "	stmdb r0!, {r4-r11}					\n"
            "	str r0, [r2]						\n"
            "										\n"
            "	stmdb sp!, {r3, r14}				\n"
            "	mov r0, %0							\n"
            "	msr basepri, r0						\n"
            "   dsb                                 \n"
            "   isb                                 \n"
            "	bl vTaskSwitchContext				\n"
            "	mov r0, #0							\n"
            "	msr basepri, r0						\n"
            "	ldmia sp!, {r3, r14}				\n"
            "										\n"
            "	ldr r1, [r3]						\n"
            "	ldr r0, [r1]						\n"
            "	ldmia r0!, {r4-r11}					\n"
            "	msr psp, r0							\n"
            "	isb									\n"
            "	bx r14								\n"
            "	nop									\n"
            "	.align 4							\n"
            "pxCurrentTCBConst: .word pxCurrentTCB	\n"
            ::"i" ( configShieldInterPriority )
            );
}

__attribute__((always_inline)) inline uint32_t  xEnterCritical( void )
{
    uint32_t xReturn;
    uint32_t temp;

    __asm volatile(
            " cpsid i               \n"
            " mrs %0, basepri       \n"
            " mov %1, %2			\n"
            " msr basepri, %1       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            : "=r" (xReturn), "=r"(temp)
            : "r" (configShieldInterPriority)
            : "memory"
            );

    return xReturn;
}

__attribute__((always_inline)) inline void xExitCritical( uint32_t xReturn )
{
    __asm volatile(
            " cpsid i               \n"
            " msr basepri, %0       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            :: "r" (xReturn)
            : "memory"
            );
}

__attribute__( ( always_inline ) ) static inline uint8_t FindHighestPriority( uint32_t Table )
{
    uint8_t temp,TopZeroNumber;
    __asm volatile
            (
            "clz %0, %2\n"
            "mov %1, #31\n"
            "sub %0, %1, %0\n"
            :"=r" (TopZeroNumber),"=r"(temp)
            :"r" (Table)
            );
    return TopZeroNumber;
}

uint8_t volatile PendSV = 0;
void vTaskSwitchContext( void )
{
    PendSV++;
    pxCurrentTCB = TcbTaskTable[ FindHighestPriority(Ready)];
}

/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    uint32_t WakeTime = TicksBase + ticks;
    TCB_t *self = pxCurrentTCB;
    if( WakeTime < TicksBase) {
        OverWakeTicksTable[self->uxPriority] = WakeTime;
    }
    else {
        WakeTicksTable[self->uxPriority] = WakeTime;
    }
    /* This is a useless operation(for Delay), it can be discarded.
     * But it is retained for the sake of specification.For example, view the status of all current tasks.*/
    Delay |= (1 << (self->uxPriority) );
    Ready &= ~(1 << (self->uxPriority) );
    switchTask();
}


void CheckTicks( void )
{
    uint32_t LookupTable = Delay;
    TicksBase += 1;
    if( TicksBase == 0){
        TicksTableSwitch( );
    }
    //find delaying Task
    while(LookupTable != 0){
        uint8_t i = FindHighestPriority(LookupTable);
        LookupTable &= ~(1 << i );
        if ( TicksBase >= WakeTicksTable[i] ) {
            WakeTicksTable[i] = 0;
            Delay &= ~(1 << i);//it is retained for the sake of specification.
            Ready |= (1 << i);
        }
    }
    switchTask();
}

void SysTick_Handler(void)
{
    uint32_t xre = xEnterCritical();
    uint32_t temp = Suspend;
    temp &= 1;// If the idle task is suspended, the scheduler is suspended
    if(temp != 1) {
        CheckTicks();
    }
    xExitCritical(xre);
}

uint32_t * pxPortInitialiseStack( uint32_t * pxTopOfStack,
                                  TaskFunction_t pxCode,
                                  void * pvParameters ,
                                  TaskHandle_t * const self)
{
    pxTopOfStack -= 16;
    Stack_register *Stack = (Stack_register *)pxTopOfStack;

    Stack->xPSR = 0x01000000UL;
    Stack->PC = ( ( uint32_t ) pxCode ) & ( ( uint32_t ) 0xfffffffeUL );
    Stack->LR = ( uint32_t ) pvParameters;
    Stack->r0 = ( uint32_t ) self;
    (*self)->self_stack = Stack;

    return pxTopOfStack;
}

void xTaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,//You can use it for debugging
                  uint32_t uxPriority,
                  TaskHandle_t * const self )
{
    uint32_t *topStack =NULL;
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t *));
    *self = ( TCB_t *) NewTcb;

    TcbTaskTable[uxPriority] = NewTcb;//

    NewTcb->uxPriority = uxPriority;
    NewTcb->pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) aligment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters,self);
    pxCurrentTCB = NewTcb;

    Ready |= (1 << uxPriority); //
}

void EnterSleepMode(void)
{
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
    __WFI();
}


//Task handle can be hide, but in order to debug, it must be created manually by the user
TaskHandle_t leisureTcb = NULL;

void leisureTask( )
{//leisureTask content can be manually modified as needed
    while (1) {
        EnterSleepMode();
    }
}

void SchedulerInit( void )
{
    TicksTableInit();
    xTaskCreate(    leisureTask,
                    128,
                    NULL,
                    0,
                    &leisureTcb
    );
}


__attribute__( ( always_inline ) )  inline void SchedulerStart( void )
{

    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 16UL );
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 24UL );

    /* Stop and clear the SysTick. */
    SysTick->CTRL = 0UL;
    SysTick->VAL = 0UL;
    /* Configure SysTick to interrupt at the requested rate. */
    SysTick->LOAD = ( configSysTickClockHz / configTickRateHz ) - 1UL;
    SysTick->CTRL = ( ( 1UL << 2UL ) | ( 1UL << 1UL ) | ( 1UL << 0UL ) );

    /* Start the first task. */
    __asm volatile (
            " ldr r0, =0xE000ED08 	\n"/* Use the NVIC offset register to locate the stack. */
            " ldr r0, [r0] 			\n"
            " ldr r0, [r0] 			\n"
            " msr msp, r0			\n"/* Set the msp back to the start of the stack. */
            " cpsie i				\n"/* Globally enable interrupts. */
            " cpsie f				\n"
            " dsb					\n"
            " isb					\n"
            " svc 0					\n"/* System call to start first task. */
            " nop					\n"
            " .ltorg				\n"
            );
}

//The abstraction layer of scheduling !!!
uint32_t StateAdd( TCB_t *self,uint32_t *State)
{
    uint32_t xre = xEnterCritical();
    (*State) |= (1 << self->uxPriority);
    xExitCritical(xre);
    return *State;
}

uint32_t StateRemove( TCB_t *self, uint32_t *State)
{
    uint32_t xre1 = xEnterCritical();
    (*State) &= ~(1 << self->uxPriority);
    xExitCritical(xre1);
    return *State;
}

uint8_t CheckState( TCB_t *self,uint32_t State )// If task is the State,return the State
{
    uint32_t xre2 = xEnterCritical();
    State &= (1 << self->uxPriority);
    xExitCritical(xre2);
    return State;
}
// the abstraction layer is end



Class(Semaphore_struct)
{
    uint8_t value;
    uint32_t Block;
};


Semaphore_struct *semaphore_creat(uint8_t value)
{
    Semaphore_struct *xSemaphore = heap_malloc(sizeof (Semaphore_struct) );
    xSemaphore->value = value;
    xSemaphore->Block = 0;
    return xSemaphore;
}

void semaphore_delete(Semaphore_struct *semaphore)
{
    heap_free(semaphore);
}

uint8_t semaphore_release( Semaphore_struct *semaphore)
{
    uint32_t xre = xEnterCritical();

    if (semaphore->Block) {
        uint8_t i =  FindHighestPriority(semaphore->Block);
        StateRemove(TcbTaskTable[i],&semaphore->Block);
        StateRemove(TcbTaskTable[i],&Block);// Also synchronize with the total blocking state
        StateRemove(TcbTaskTable[i],&Delay);
        StateAdd(TcbTaskTable[i], &Ready);
    }
    semaphore->value++;
    switchTask();

    xExitCritical(xre);
    return true;
}

uint8_t semaphore_take(Semaphore_struct *semaphore,uint32_t Ticks)
{
    uint32_t xre = xEnterCritical();

    if( semaphore->value > 0) {
        semaphore->value--;
        switchTask();
        xExitCritical(xre);
        return true;
    }

    if(Ticks == 0 ){
        return false;
    }

    uint8_t volatile temp = PendSV;
    if(Ticks > 0){
        StateAdd(pxCurrentTCB,&Block);
        StateAdd(pxCurrentTCB,&semaphore->Block);
        TaskDelay(Ticks);
    }
    xExitCritical(xre);

    while(temp == PendSV){ }//It loops until the schedule is start.

    uint32_t xReturn = xEnterCritical();
    //Check whether the wake is due to delay or due to semaphore availability
    if( CheckState(pxCurrentTCB,Block) ){//if true ,the task is Block!
        StateRemove(pxCurrentTCB,&semaphore->Block);
        StateRemove(pxCurrentTCB,&Block);
        xExitCritical(xReturn);
        return false;
    }else{
        semaphore->value--;
        switchTask();
        xExitCritical(xReturn);
        return true;
    }
}

//Task Area!The user must create task handle manually because of debugging and specification
