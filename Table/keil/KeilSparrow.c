#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>



#define alignment_byte               0x07
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )
#define configShieldInterPriority 	191
#define config_heap   8*1024
#define configMaxPriority  32

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



void *heap_malloc(size_t WantSize)
{
    heap_node *prev_node;
    heap_node *use_node;
    heap_node *new_node;
    size_t alignment_require_size;
    void *xReturn = NULL;
    WantSize += HeapStructSize;
    if((WantSize & alignment_byte) != 0x00) {
        alignment_require_size = (alignment_byte + 1) - (WantSize & alignment_byte);//must 8-byte alignment
        WantSize += alignment_require_size;
    }//You can add the TaskSuspend function ,that make here be an atomic operation
    if(TheHeap.tail== NULL ) {
        heap_init();
    }//Resume
    prev_node = &TheHeap.head;
    use_node = TheHeap.head.next;
    while((use_node->BlockSize) < WantSize) {//check the size is fit
        prev_node = use_node;
        use_node = use_node->next;
        if(use_node == NULL){
            return xReturn;
        }
    }
    xReturn = (void*)( ( (uint8_t*)use_node ) + HeapStructSize );
    prev_node->next = use_node->next ;
    if( (use_node->BlockSize - WantSize) > MIN_size ) {
        new_node = (void *) (((uint8_t *) use_node) + WantSize);
        new_node->BlockSize = use_node->BlockSize - WantSize;
        use_node->BlockSize = WantSize;
        new_node->next = prev_node->next;
        prev_node->next = new_node;
    }//Finish cutting
    TheHeap.AllSize-= use_node->BlockSize;
    use_node->next = NULL;
    return xReturn;
}

static void InsertFreeBlock(heap_node* xInsertBlock);
void heap_free(void *xReturn)
{
    heap_node *xlink;
    uint8_t *xFree = (uint8_t*)xReturn;

    xFree -= HeapStructSize;//get the start address of the heap struct
    xlink = (void*)xFree;
    TheHeap.AllSize += xlink->BlockSize;
    InsertFreeBlock((heap_node*)xlink);
}

static void InsertFreeBlock(heap_node* xInsertBlock)
{
    heap_node *first_fit_node;
    uint8_t* getaddr;

    for(first_fit_node = &TheHeap.head;first_fit_node->next < xInsertBlock;first_fit_node = first_fit_node->next)
    { /*finding the fit node*/ }

    xInsertBlock->next = first_fit_node->next;
    first_fit_node->next = xInsertBlock;

    getaddr = (uint8_t*)xInsertBlock;
    if((getaddr + xInsertBlock->BlockSize) == (uint8_t*)(xInsertBlock->next)) {
        if (xInsertBlock->next != TheHeap.tail) {
            xInsertBlock->BlockSize += xInsertBlock->next->BlockSize;
            xInsertBlock->next = xInsertBlock->next->next;
        } else {
            xInsertBlock->next = TheHeap.tail;
        }
    }
    getaddr = (uint8_t*)first_fit_node;
    if((getaddr + first_fit_node->BlockSize) == (uint8_t*) xInsertBlock) {
        first_fit_node->BlockSize += xInsertBlock->BlockSize;
        first_fit_node->next = xInsertBlock->next;
    }
}




Class(Stack_register)
{
    //manual stacking
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    //automatic stacking
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

__attribute__((always_inline)) void xExitCritical( uint32_t xReturn )
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
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t));
    *self = ( TCB_t *) NewTcb;
    TcbTaskTable[uxPriority] = NewTcb;
    NewTcb->uxPriority = uxPriority;
    NewTcb->pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) alignment_byte)));
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

