//
// Created by el on 2024/11/9.
//
#include "schedule.h"
#include<stdint.h>
#include "heap.h"

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
    uint8_t uxPriority;
    uint32_t * pxStack;
    Stack_register *self_stack;//Save the status of the stack in the task !You can use gdb to debug it!
};


__attribute__( ( used ) )  TaskHandle_t volatile schedule_currentTCB = NULL;


#define switchTask()\
*( ( volatile uint32_t * ) 0xe000ed04 ) = 1UL << 28UL

__attribute__( ( always_inline ) ) inline void schedule( void )
{
    switchTask();
}

uint32_t  NextTicks = ~(uint32_t)0;
uint32_t  TicksBase = 0;

TaskHandle_t TcbTaskTable[configMaxPriori];

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


//the table is defined for signal mechanism
uint32_t StateTable[5] = {0,0,0,0,0};



#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler

__attribute__( ( naked ) ) void   vPortSVCHandler( void )
{
    __asm volatile (
            "	ldr	r3, schedule_currentTCBConst2		\n"
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
            "schedule_currentTCBConst2: .word schedule_currentTCB				\n"
            );
}

void __attribute__( ( naked ) )  xPortPendSVHandler( void )
{
    __asm volatile
            (
            "	mrs r0, psp							\n"
            "	isb									\n"
            "										\n"
            "	ldr	r3, schedule_currentTCBConst			\n"
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
            "schedule_currentTCBConst: .word schedule_currentTCB	\n"
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


__attribute__( ( always_inline ) )  inline uint8_t FindHighestPriority( uint32_t Table )
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

#if IPC
uint8_t volatile PendSV = 0;
#endif

void vTaskSwitchContext( void )
{
    #if IPC
    PendSV++;
    #endif
    schedule_currentTCB = TcbTaskTable[ FindHighestPriority(StateTable[Ready])];
}


/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    uint32_t WakeTime = TicksBase + ticks;
    TCB_t *self = schedule_currentTCB;
    if( WakeTime < TicksBase) {
        OverWakeTicksTable[self->uxPriority] = WakeTime;
    }
    else {
        WakeTicksTable[self->uxPriority] = WakeTime;
    }
    /* This is a useless operation(for Delay), it can be discarded.
     * But it is retained for the sake of specification.For example, view the status of all current tasks.*/
    StateTable[Delay] |= (1 << (self->uxPriority) );
    StateTable[Ready] &= ~(1 << (self->uxPriority) );
    switchTask();
}


void CheckTicks( void )
{
    uint32_t LookupTable = StateTable[Delay];
    TicksBase += 1;
    if( TicksBase == 0){
        TicksTableSwitch( )
    }
    //find delaying Task
    while(LookupTable != 0){
        uint8_t i = FindHighestPriority(LookupTable);
        LookupTable &= ~(1 << i );
        if ( TicksBase >= WakeTicksTable[i] ) {
            WakeTicksTable[i] = 0;
            StateTable[Delay] &= ~(1 << i);//it is retained for the sake of specification.
            StateTable[Ready] |= (1 << i);
        }
    }
    switchTask();
}


void SysTick_Handler(void)
{
    uint32_t xre = xEnterCritical();
    uint32_t temp = StateTable[Suspend];
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
    uint32_t *topStack = NULL;
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t *));
    *self = ( TCB_t *) NewTcb;

    TcbTaskTable[uxPriority] = NewTcb;//

    NewTcb->uxPriority = uxPriority;
    NewTcb->pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) alignment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters,self);
    schedule_currentTCB = NewTcb;

    StateTable[Ready] |= (1 << uxPriority); //
}

Class(SCB_Type)
{
    uint32_t CPUID;                  /*!< Offset: 0x000 (R/ )  CPUID Base Register */
    uint32_t ICSR;                   /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register */
    uint32_t VTOR;                   /*!< Offset: 0x008 (R/W)  Vector Table Offset Register */
    uint32_t AIRCR;                  /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
    uint32_t SCR;                    /*!< Offset: 0x010 (R/W)  System Control Register */
    uint32_t CCR;                    /*!< Offset: 0x014 (R/W)  Configuration Control Register */
    uint8_t  SHP[12U];               /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
    uint32_t SHCSR;                  /*!< Offset: 0x024 (R/W)  System Handler Control and State Register */
    uint32_t CFSR;                   /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register */
    uint32_t HFSR;                   /*!< Offset: 0x02C (R/W)  HardFault Status Register */
    uint32_t DFSR;                   /*!< Offset: 0x030 (R/W)  Debug Fault Status Register */
    uint32_t MMFAR;                  /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register */
    uint32_t BFAR;                   /*!< Offset: 0x038 (R/W)  BusFault Address Register */
    uint32_t AFSR;                   /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register */
    uint32_t PFR[2U];                /*!< Offset: 0x040 (R/ )  Processor Feature Register */
    uint32_t DFR;                    /*!< Offset: 0x048 (R/ )  Debug Feature Register */
    uint32_t ADR;                    /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register */
    uint32_t MMFR[4U];               /*!< Offset: 0x050 (R/ )  Memory Model Feature Register */
    uint32_t ISAR[5U];               /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register */
    uint32_t RESERVED0[5U];
    uint32_t CPACR;                  /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
};


void EnterSleepMode(void)
{
    SCB_Type *SCB;
    SCB =  (SCB_Type * volatile)0xE000ED00UL;
    SCB->SCR &= ~(1UL << 2UL) ;
    __asm volatile ("wfi");
}


//Task handle can be hide, but in order to debug, it must be created manually by the user
TaskHandle_t leisureTcb = NULL;

void leisureTask( void )
{//leisureTask content can be manually modified as needed
    while (1) {
        EnterSleepMode();
    }
}

void SchedulerInit( void )
{
    TicksTableInit()
    xTaskCreate(    (TaskFunction_t)leisureTask,
                    128,
                    NULL,
                    0,
                    &leisureTcb
    );
}

Class(SysTicks){
     uint32_t CTRL;
     uint32_t LOAD;
     uint32_t VAL;
     uint32_t CALIB;

};


__attribute__( ( always_inline ) )  inline void SchedulerStart( void )
{

    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 16UL );
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 24UL );

    SysTicks *SysTick = ( SysTicks * volatile)0xe000e010;

    /* Stop and clear the SysTick. */
    *SysTick = (SysTicks){
        .CTRL = 0UL,
        .VAL  = 0UL,
    };
    /* Configure SysTick to interrupt at the requested rate. */
    *SysTick = (SysTicks){
        .LOAD = ( configSysTickClockHz / configTickRateHz ) - 1UL,
        .CTRL  = ( ( 1UL << 2UL ) | ( 1UL << 1UL ) | ( 1UL << 0UL ) )
    };

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
TaskHandle_t GetTaskHandle( uint8_t i)
{
    return TcbTaskTable[i];
}

uint8_t GetTaskPriority( TaskHandle_t taskHandle)
{
    return taskHandle->uxPriority;
}



uint32_t StateAdd( TaskHandle_t taskHandle,uint8_t State)
{
    uint32_t xre = xEnterCritical();
    StateTable[State] |= (1 << taskHandle->uxPriority);
    xExitCritical(xre);
    return StateTable[State];
}

uint32_t StateRemove( TaskHandle_t taskHandle, uint8_t State)
{
    uint32_t xre1 = xEnterCritical();
    StateTable[State] &= ~(1 << taskHandle->uxPriority);
    xExitCritical(xre1);
    return StateTable[State];
}

uint8_t CheckState( TaskHandle_t taskHandle,uint8_t State )// If task is the State,return the State
{
    uint32_t xre2 = xEnterCritical();
    StateTable[State] &= (1 << taskHandle->uxPriority);
    xExitCritical(xre2);
    return StateTable[State];
}

// the abstraction layer is end
