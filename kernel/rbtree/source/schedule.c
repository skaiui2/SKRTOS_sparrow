/*
 * MIT License
 *
 * Copyright (c) 2024 skaiui2

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *  https://github.com/skaiui2/SKRTOS_sparrow
 */


#include <memory.h>
#include "schedule.h"
#include "heap.h"
#include "port.h"
#include "rbtree.h"


Class(TCB_t)
{
    volatile uint32_t *pxTopOfStack;
    rb_node task_node;
    rb_node IPC_node;
    uint8_t state;
    uint8_t uxPriority;
    uint32_t * pxStack;
};

__attribute__( ( used ) )  TaskHandle_t volatile schedule_currentTCB = NULL;

__attribute__( ( always_inline ) ) inline TaskHandle_t GetCurrentTCB(void)
{
    return schedule_currentTCB;
}


__attribute__( ( always_inline ) ) inline TaskHandle_t TaskHighestPriority(rb_root_handle root)
{
    rb_node *rb_highest_node = root->last_node;
    //rb_node *rb_highest_node = rb_last(root);
    return container_of(rb_highest_node, TCB_t, task_node);
}

__attribute__( ( always_inline ) ) inline TaskHandle_t IPCHighestPriorityTask(rb_root_handle root)
{
    rb_node *rb_highest_node = root->last_node;
    return container_of(rb_highest_node, TCB_t, IPC_node);
}

uint8_t GetTaskPriority(TaskHandle_t taskHandle)
{
    return taskHandle->uxPriority;
}



rb_root ReadyTree;
rb_root OneDelayTree;
rb_root TwoDelayTree;
rb_root *WakeTicksTree;
rb_root *OverWakeTicksTree;
rb_root SuspendTree;
rb_root DeleteTree;


static volatile uint32_t NowTickCount = ( uint32_t ) 0;



void ReadyTreeAdd(rb_node *node)
{
    TaskHandle_t self = container_of(node, TCB_t, task_node);
    node->value = self->uxPriority;
    rb_Insert_node( &ReadyTree, node);
}


void ReadyTreeRemove(rb_node *node)
{
    rb_remove_node(&ReadyTree, node);
}

void SuspendTreeAdd(rb_node *node)
{
    rb_Insert_node( &SuspendTree, node);
}

void SuspendTreeRemove(rb_node *node)
{
    rb_remove_node( &SuspendTree, node);
}



void TaskTreeAdd(TaskHandle_t self, uint8_t State)
{
    uint32_t xReturn = xEnterCritical();
    rb_node *node = &(self->task_node);
    void (*TreeAdd[])(rb_node *node) = {
            ReadyTreeAdd,
            SuspendTreeAdd
    };
    TreeAdd[State](node);
    xExitCritical(xReturn);
}



void TaskTreeRemove(TaskHandle_t self, uint8_t State)
{
    uint32_t xReturn = xEnterCritical();
    rb_node *node = &(self->task_node);
    void (*TreeRemove[])(rb_node *node) = {
            ReadyTreeRemove,
            SuspendTreeRemove
    };
    TreeRemove[State](node);
    xExitCritical(xReturn);
}

void Insert_IPC(TaskHandle_t self, rb_root *root)
{
    self->IPC_node.root = root;
    self->IPC_node.value = self->uxPriority;
    rb_Insert_node(root, &(self->IPC_node));
}


void Remove_IPC(TaskHandle_t self)
{
    rb_remove_node( self->IPC_node.root , &(self->IPC_node));
}



void ADTTreeInit(void)
{
    rb_root_init(&ReadyTree);
    rb_root_init(&SuspendTree);
    rb_root_init(&DeleteTree);
}




struct Stack_register {
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


#define switchTask()\
*( ( volatile uint32_t * ) 0xe000ed04 ) = 1UL << 28UL

__attribute__( ( always_inline ) ) inline void schedule( void )
{
    switchTask();
}


__attribute__((always_inline)) inline void StateSet( TaskHandle_t taskHandle,uint8_t State)
{
    taskHandle->state = State;
}

__attribute__((always_inline)) inline uint8_t CheckIPCState( TaskHandle_t taskHandle)
{
    return taskHandle->IPC_node.root == NULL;
}

__attribute__((always_inline)) inline uint8_t CheckTaskState( TaskHandle_t taskHandle, uint8_t State)// If task is the State,return true
{
    return taskHandle->state == State;
}

// the abstraction layer is end

uint8_t volatile schedule_PendSV = 0;

void vTaskSwitchContext( void )
{
    schedule_PendSV++;
    schedule_currentTCB = TaskHighestPriority(&ReadyTree);
}


void RecordWakeTime(uint16_t ticks)
{
    const uint32_t constTicks = NowTickCount;
    TCB_t *self = schedule_currentTCB;
    self->task_node.value = constTicks + ticks;

    if( self->task_node.value < constTicks) {
        rb_Insert_node(OverWakeTicksTree, &(self->task_node));
    } else {
        rb_Insert_node(WakeTicksTree, &(self->task_node));
    }
}


/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    TaskTreeRemove(schedule_currentTCB,Ready);
    RecordWakeTime(ticks);
    schedule();
}

/*
 * If the program runs here, there is a problem with the use of the RTOS,
 * such as the stack allocation space is too small, and the use of undefined operations
 */
void ErrorHandle(void)
{
    while (1){

    }
}



uint32_t * pxPortInitialiseStack( uint32_t * pxTopOfStack,
                                  TaskFunction_t pxCode,
                                  void * pvParameters)
{
    pxTopOfStack -= 16;
    struct Stack_register *Stack = (struct Stack_register *)pxTopOfStack;

    Stack->xPSR = 0x01000000UL;
    Stack->PC = ( ( uint32_t ) pxCode ) & ( ( uint32_t ) 0xfffffffeUL );
    Stack->LR = ( uint32_t ) ErrorHandle;
    Stack->r0 = ( uint32_t ) pvParameters;

    return pxTopOfStack;
}



void  TaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,//You can use it for debugging
                  uint32_t uxPriority,
                  TaskHandle_t * const self
                  )
{
    uint32_t *topStack = NULL;
    uint32_t *pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t));
    memset( ( void * ) NewTcb, 0x00, sizeof( TCB_t ) );
    *self = ( TCB_t *) NewTcb;
    *NewTcb = (TCB_t){
        .state = Ready,
        .uxPriority = uxPriority,
        .pxStack = pxStack
    };
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) alignment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters);
    rb_node_init(&NewTcb->task_node);
    rb_node_init(&NewTcb->IPC_node);
    TaskTreeAdd(NewTcb, Ready);
}

void TaskDelete(TaskHandle_t self)
{
    TaskTreeRemove(self, Ready);
    rb_Insert_node(&DeleteTree, &self->task_node);
    schedule();
}


void TreeDelayInit(void)
{
    rb_root_init(&OneDelayTree);
    rb_root_init(&TwoDelayTree);
    WakeTicksTree = NULL;
    OverWakeTicksTree = NULL;
    WakeTicksTree = &OneDelayTree;
    OverWakeTicksTree = &TwoDelayTree;
}


void TaskFree(void)
{
    if (DeleteTree.count != 0) {
        rb_node *first_node = rb_last(&DeleteTree);
        TaskHandle_t self = container_of(first_node, TCB_t, task_node);
        rb_remove_node(&DeleteTree, &self->task_node);
        heap_free((void *)self->pxStack);
        heap_free((void *)self);
    }
}

//Task handle can be hide, but in order to debug, it must be created manually by the user
TaskHandle_t leisureTcb = NULL;

void leisureTask( void )
{//leisureTask content can be manually modified as needed
    while (1) {
        TaskFree();
    }
}

void LeisureTaskCreat(void)
{
    TaskCreate(    (TaskFunction_t)leisureTask,
                    128,
                    NULL,
                    0,
                    &leisureTcb );
}


void SchedulerInit(void)
{
    ADTTreeInit();
    TreeDelayInit();
    LeisureTaskCreat();
}


struct SysTicks {
    uint32_t CTRL;
    uint32_t LOAD;
    uint32_t VAL;
    uint32_t CALIB;
};


void PendSvInit(void)
{
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 16UL );
}

void SysTickInit(void)
{
    ( *( ( volatile uint32_t * ) 0xe000ed20 ) ) |= ( ( ( uint32_t ) 255UL ) << 24UL );

    struct SysTicks *SysTick = ( struct SysTicks * volatile)0xe000e010;

    /* Stop and clear the SysTick. */
    *SysTick = (struct SysTicks){
            .CTRL = 0UL,
            .VAL  = 0UL,
    };
    /* Configure SysTick to interrupt at the requested rate. */
    *SysTick = (struct SysTicks){
            .LOAD = ( configSysTickClockHz / configTickRateHz ) - 1UL,
            .CTRL  = ( ( 1UL << 2UL ) | ( 1UL << 1UL ) | ( 1UL << 0UL ) )
    };
}


void HandlerInit(void)
{
    PendSvInit();
    SysTickInit();
}

__attribute__( ( always_inline ) )  inline void SchedulerStart( void )
{
    HandlerInit();
    vTaskSwitchContext();
    StartFirstTask();
}


void DelayTreeRemove(TaskHandle_t self)
{
    rb_remove_node(WakeTicksTree, &(self->task_node));
}



void CheckTicks(void)
{
    rb_node *rb_node = NULL;
    NowTickCount++;

    if( NowTickCount == ( uint32_t) 0UL) {
        rb_root *temp;
        temp = WakeTicksTree;
        WakeTicksTree = OverWakeTicksTree;
        OverWakeTicksTree = temp;
    }

    while ( (rb_node = WakeTicksTree->first_node) && (rb_node->value <= NowTickCount)) {
        TaskHandle_t self = container_of(rb_node, TCB_t, task_node);
        DelayTreeRemove(self);
        TaskTreeAdd(self, Ready);
    }

    schedule();
}



volatile uint64_t AbsoluteClock = 0;
void SysTick_Handler(void)
{
    uint32_t xre = xEnterCritical();
    AbsoluteClock++;
    // If the idle task is suspended, the scheduler is suspended
    if(CheckTaskState(leisureTcb, Suspend) == 0) {
        CheckTicks();
    }
    xExitCritical(xre);
}



