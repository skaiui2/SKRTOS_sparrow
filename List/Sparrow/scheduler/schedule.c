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


Class(TCB_t)
{
    volatile uint32_t *pxTopOfStack;
    ListNode task_node;
    ListNode IPC_node;
    uint8_t state;
    uint8_t uxPriority;
    uint32_t * pxStack;
    uint8_t TimeSlice;
};

__attribute__( ( used ) )  TaskHandle_t volatile schedule_currentTCB = NULL;

__attribute__( ( always_inline ) ) inline TaskHandle_t GetCurrentTCB(void)
{
    return schedule_currentTCB;
}


__attribute__( ( always_inline ) ) inline TaskHandle_t TaskHighestPriorityTask(TheList *xlist)
{
    return container_of(xlist->tail, TCB_t, task_node);
}

__attribute__( ( always_inline ) ) inline TaskHandle_t IPCHighestPriorityTask(TheList *xlist)
{
    return container_of(xlist->tail, TCB_t, IPC_node);
}

uint8_t GetTaskPriority(TaskHandle_t taskHandle)
{
    return taskHandle->uxPriority;
}



TheList ReadyListArray[configMaxPriority];
TheList OneDelayList;
TheList TwoDelayList;
TheList *WakeTicksList;
TheList *OverWakeTicksList;

static volatile uint32_t NowTickCount = ( uint32_t ) 0;

TheList SuspendList;
TheList BlockList;
TheList DeleteList;

void ReadyListInit( void )
{
    uint8_t i = configMaxPriority - 1;
    while( i != 0)
    {
        ListInit(&(ReadyListArray[i]));
        i--;
    }
}



static void ReadyListAdd(ListNode *node)
{
    TaskHandle_t self = container_of(node, TCB_t, task_node);
    self->task_node.value = self->TimeSlice;
    ListAdd( &(ReadyListArray[self->uxPriority]), node);
}


static void ReadyListRemove(ListNode *node)
{
    TaskHandle_t self = container_of(node, TCB_t, task_node);
    ListRemove( &(ReadyListArray[self->uxPriority]), node);
}

static void SuspendListAdd(ListNode *node)
{
    ListAdd( &SuspendList, node);
}

static void SuspendListRemove(ListNode *node)
{
    ListRemove( &SuspendList, node);
}


void TaskListAdd(TaskHandle_t self, uint8_t State)
{
    uint32_t xReturn = xEnterCritical();
    ListNode *node = &(self->task_node);
    void (*ListAdd[])(ListNode *node) = {
            ReadyListAdd,
            SuspendListAdd
    };
    ListAdd[State](node);
    xExitCritical(xReturn);
}



void TaskListRemove(TaskHandle_t self, uint8_t State)
{
    uint32_t xReturn = xEnterCritical();
    ListNode *node = &(self->task_node);
    void (*ListRemove[])(ListNode *node) = {
            ReadyListRemove,
            SuspendListRemove
    };
    ListRemove[State](node);
    xExitCritical(xReturn);
}

void Insert_IPC(TaskHandle_t self,TheList *IPC_list)
{
    self->IPC_node.value = self->uxPriority;
    ListAdd( IPC_list , &(self->IPC_node));
}


void Remove_IPC(TaskHandle_t self)
{
    ListRemove( self->IPC_node.TheList , &(self->IPC_node));
    self->IPC_node.value = 0;
}

static uint8_t ListHighestPriorityTask(void)
{
    uint8_t i = configMaxPriority - 1;
    while(ReadyListArray[i].count == 0){
        i--;
    }
    return i;
}


void ADTListInit(void)
{
    ReadyListInit();
    ListInit( &SuspendList );
    ListInit( &BlockList );
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
    return taskHandle->IPC_node.TheList == NULL;
}

__attribute__((always_inline)) inline uint8_t CheckTaskState( TaskHandle_t taskHandle, uint8_t State)// If task is the State,return true
{
    return taskHandle->state == State;
}

// the abstraction layer is end

uint8_t volatile schedule_PendSV = 0;

void vTaskSwitchContext( void )
{
    uint8_t Index= ListHighestPriorityTask();
    TheList *TopPrioritiesList = &(ReadyListArray[Index]);
    if( TopPrioritiesList->SwitchFlag > 0)
    {
        TopPrioritiesList->SwitchFlag -= 1;
    }else {
        TopPrioritiesList->SaveNode = TopPrioritiesList->SaveNode->next;
        TopPrioritiesList->SwitchFlag = TopPrioritiesList->SaveNode->value;
    }

    schedule_PendSV++;
    schedule_currentTCB = container_of(TopPrioritiesList->SaveNode,TCB_t ,task_node);
}


void RecordWakeTime(uint16_t ticks)
{
    const uint32_t constTicks = NowTickCount;
    TCB_t *self = schedule_currentTCB;
    self->task_node.value = constTicks + ticks;

    if( self->task_node.value < constTicks) {
        ListAdd(OverWakeTicksList, &(self->task_node));
    } else {
        ListAdd(WakeTicksList, &(self->task_node));
    }
}


/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    TaskListRemove(schedule_currentTCB,Ready);
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



void TaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,//You can use it for debugging
                  uint32_t uxPriority,
                  TaskHandle_t * const self,
                  uint8_t TimeSlice)
{
    uint32_t *topStack = NULL;
    uint32_t *pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t));
    memset( ( void * ) NewTcb, 0x00, sizeof( TCB_t ) );
    *self = ( TCB_t *) NewTcb;
    *NewTcb = (TCB_t){
        .state = Ready,
        .uxPriority = uxPriority,
        .TimeSlice = TimeSlice,
        .pxStack = pxStack
    };
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) alignment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters);
    TaskListAdd(NewTcb, Ready);
}

void TaskDelete(TaskHandle_t self)
{
    TaskListRemove(self, Ready);
    ListAdd(&DeleteList, &self->task_node);
    schedule();
}


void ListDelayInit(void)
{
    ListInit( &OneDelayList );
    ListInit( &TwoDelayList);
    WakeTicksList = &OneDelayList;
    OverWakeTicksList = &TwoDelayList;
}


void TaskFree(void)
{
    if (DeleteList.count != 0) {
        TaskHandle_t self = container_of(DeleteList.head, TCB_t, task_node);
        ListRemove(&DeleteList, &self->task_node);
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
                    &leisureTcb,
                    0
    );
}


void SchedulerInit(void)
{
    ADTListInit();
    ListDelayInit();
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


void DelayListRemove(TaskHandle_t self)
{
    ListRemove(WakeTicksList, &(self->task_node));
}



void CheckTicks(void)
{
    ListNode *list_node = NULL;
    NowTickCount++;

    if( NowTickCount == ( uint32_t) 0UL) {
        TheList *temp;
        temp = WakeTicksList;
        WakeTicksList = OverWakeTicksList;
        OverWakeTicksList = temp;
    }

    while ( (list_node = WakeTicksList->head) && (list_node->value <= NowTickCount) ) {
        TaskHandle_t self = container_of(list_node, TCB_t, task_node);
        DelayListRemove(self);
        TaskListAdd(self, Ready);
    }

    schedule();
}



extern uint64_t AbsoluteClock;
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



