#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>


#define aligment_byte               0x07
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )
#define configShieldInterPriority 	191
#define config_heap   8*1024
#define configMaxPriority  32


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
    }
    theheap.head.next = (heap_node *)start_heap;
    theheap.head.blocksize = (size_t)0;
    end_heap  = ( start_heap + (uint32_t)config_heap);
    if( (end_heap & aligment_byte) != 0){
        end_heap += aligment_byte ;
        end_heap &= ~aligment_byte;
        theheap.allsize =  (size_t)(end_heap - start_heap );
    }
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
    if((wantsize & aligment_byte) != 0x00)
    {
        aligmentrequisize = aligment_byte - (wantsize & aligment_byte);
        wantsize += aligmentrequisize;
    }//I will add the TaskSuspend function ,that make here be a atomic operation
    if(theheap.tail== NULL )
    {
        heap_init();
    }//
    prevnode = &theheap.head;
    usenode = theheap.head.next;
    while((usenode->blocksize) < wantsize )
    {
        prevnode = usenode;
        usenode = usenode ->next;
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
void heap_free(void *xret)
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


#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

__asm void vPortSVCHandler( void )
{
	  PRESERVE8

	  ldr	r3, =pxCurrentTCB	/* Restore the context. */
	  ldr r1, [r3]			/* Use pxCurrentTCBConst to get the pxCurrentTCB address. */
	  ldr r0, [r1]			/* The first item in pxCurrentTCB is the task top of stack. */
	  ldmia r0!, {r4-r11}		/* Pop the registers that are not automatically saved on exception entry and the critical nesting count. */
	  msr psp, r0				/* Restore the task stack pointer. */
	  isb
	  mov r0, #0
	  msr	basepri, r0
	  orr r14, #0xd
	  bx r14
}

__asm void xPortPendSVHandler( void )
{
	  extern pxCurrentTCB;
	  extern vTaskSwitchContext;

	  PRESERVE8

	  mrs r0, psp
	  isb

	  ldr	r3, =pxCurrentTCB		/* Get the location of the current TCB. */
	  ldr	r2, [r3]

	  stmdb r0!, {r4-r11}			/* Save the remaining registers. */
	  str r0, [r2]				/* Save the new top of stack into the first member of the TCB. */

	  stmdb sp!, {r3, r14}
	  mov r0, configShieldInterPriority
	  msr basepri, r0
	  dsb
	  isb
	  bl vTaskSwitchContext
	  mov r0, #0
	  msr basepri, r0
	  ldmia sp!, {r3, r14}

  	ldr r1, [r3]
	  ldr r0, [r1]				/* The first item in pxCurrentTCB is the task top of stack. */
	  ldmia r0!, {r4-r11}			/* Pop the registers and the critical nesting count. */
	  msr psp, r0
	  isb
	  bx r14
	  nop
}

#define switchTask()\
*( ( volatile uint32_t * ) 0xe000ed04 ) = 1UL << 28UL;

uint32_t  NextTicks = ~(uint32_t)0;
uint32_t  TicksBase = 0;

uint32_t ReadyBitTable = 0;
TaskHandle_t TcbTaskTable[configMaxPriority];

uint32_t DelayBitTable = 0;
uint32_t TicksTable[configMaxPriority];
uint32_t TicksTableAssist[configMaxPriority];
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

uint32_t SuspendBitTable = 0;

//the table is defined for signal mechanism
uint32_t BlockedBitTable = 0;

/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    uint32_t WakeTime = TicksBase + ticks;
    TCB_t *self = pxCurrentTCB;
    if( WakeTime < TicksBase)
    {
        OverWakeTicksTable[self->uxPriority] = WakeTime;
    }
    else
    {
        WakeTicksTable[self->uxPriority] = WakeTime;
    }
    /* This is a useless operation(for DelayBitTable), it can be discarded.
     * But it is retained for the sake of normativity.For example, view the status of all current tasks.*/
    DelayBitTable |= (1 << (self->uxPriority) );
    ReadyBitTable &= ~(1 << (self->uxPriority) );
    switchTask();
}

void CheckTicks( void )
{
    TicksBase += 1;
    if( TicksBase == 0){
        TicksTableSwitch( );
    }
    for(int i=0 ; i < configMaxPriority;i++) {
        if(  WakeTicksTable[i]  > 0) {
            if ( TicksBase >= WakeTicksTable[i] ) {
                WakeTicksTable[i] = 0;
                DelayBitTable &= ~(1 << i );//it is retained for the sake of normativity.
                ReadyBitTable |= (1 << i);
            }
        }
    }
    switchTask();
}



__attribute__((always_inline)) uint32_t xEnterCritical( void )
{
    uint32_t xReturn;
	uint32_t BarrierPriority = configShieldInterPriority;

    __asm volatile(
            " cpsid i               \n"
            " mrs xReturn, basepri       \n"
            " msr basepri, BarrierPriority       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            );

    return xReturn;
}

__attribute__((always_inline)) void xEixtCritical( uint32_t xReturn )
{
    __asm volatile(
            " cpsid i               \n"
            " msr basepri, xReturn       \n"
            " dsb                   \n"
            " isb                   \n"
            " cpsie i               \n"
            );
}


void SysTick_Handler(void)
{
    uint32_t xre = xEnterCritical();
    CheckTicks();
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
                  void * const pvParameters,//it can be used to debug
                  uint32_t uxPriority,
                  TaskHandle_t * const self )
{
    uint32_t *topStack =NULL;
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t *));
    *self = ( TCB_t *) NewTcb;
    TcbTaskTable[uxPriority] = NewTcb;
    NewTcb->uxPriority = uxPriority;
    NewTcb->pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) aligment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters,self);

    pxCurrentTCB = NewTcb;
    ReadyBitTable |= (1 << uxPriority);
}

__attribute__((always_inline)) static inline uint8_t FindHighestPriority(void) {
    uint8_t TopZeroNumber;
    uint8_t temp;
    __asm {
        clz TopZeroNumber, ReadyBitTable
        mov temp, #31
        sub TopZeroNumber, temp, TopZeroNumber
    }
    return TopZeroNumber;
}


void vTaskSwitchContext( void )
{
    pxCurrentTCB = TcbTaskTable[ FindHighestPriority()];
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


__asm void prvStartFirstTask( void )
{
	  PRESERVE8
	/* Use the NVIC offset register to locate the stack. */
	  ldr r0, =0xE000ED08
	  ldr r0, [r0]
  	ldr r0, [r0]
	/* Set the msp back to the start of the stack. */
  	msr msp, r0
	/* Globally enable interrupts. */
	  cpsie i
	  cpsie f
	  dsb
	  isb
	/* Call SVC to start the first task. */
	  svc 0
	  nop
	  nop
}

__attribute__( ( always_inline ) ) inline void SchedulerStart( void )
{
    /* Start the timer that generates the tick ISR.  Interrupts are disabled
     * here already. */
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 16UL );
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 24UL );

    /* Stop and clear the SysTick. */
    SysTick->CTRL = 0UL;
    SysTick->VAL = 0UL;
    /* Configure SysTick to interrupt at the requested rate. */
    SysTick->LOAD = ( configSysTickClockHz / configTickRateHz ) - 1UL;
    SysTick->CTRL = ( ( 1UL << 2UL ) | ( 1UL << 1UL ) | ( 1UL << 0UL ) );

    /* Start the first task. */
    prvStartFirstTask();
}

//Task Area!The user must create task handle manually because of debugging and specification

