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

void ReadyListInit( void )
{
    uint8_t i = configMaxPriority - 1;
    while( i != 0)
    {
        ListInit(&(ReadyListArray[i]));
        i--;
    }
}



void ReadyListAdd(ListNode *node)
{
    TaskHandle_t self = container_of(node, TCB_t, task_node);
    self->task_node.value = self->TimeSlice;
    ListAdd( &(ReadyListArray[self->uxPriority]), node);
}


void ReadyListRemove(ListNode *node)
{
    TaskHandle_t self = container_of(node, TCB_t, task_node);
    ListRemove( &(ReadyListArray[self->uxPriority]), node);
}

void SuspendListAdd(ListNode *node)
{
    ListAdd( &SuspendList, node);
}

void SuspendListRemove(ListNode *node)
{
    ListRemove( &SuspendList, node);
}


void BlockListAdd(ListNode *node)
{
    ListAdd( &BlockList, node);
}


void BlockListRemove(ListNode *node)
{
    ListRemove( &BlockList, node);
}


void TaskListAdd(TaskHandle_t self, uint8_t State)
{
    uint32_t xReturn = xEnterCritical();
    ListNode *node = &(self->task_node);
    void (*ListAdd[])(ListNode *node) = {
            ReadyListAdd,
            SuspendListAdd,
            BlockListAdd

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
            SuspendListRemove,
            BlockListRemove
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
    uint32_t wakeTime = constTicks + ticks;
    self->task_node.value = wakeTime;

    if( wakeTime < constTicks)
    {
        ListAdd(OverWakeTicksList , &(self->task_node) );
    }
    else{
        ListAdd( WakeTicksList , &(self->task_node) );
    }
}


/*The RTOS delay will switch the task.It is used to liberate low-priority task*/
void TaskDelay( uint16_t ticks )
{
    TaskListRemove(schedule_currentTCB,Ready);
    RecordWakeTime(ticks);
    schedule();
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

    return pxTopOfStack;
}



void xTaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,//You can use it for debugging
                  uint32_t uxPriority,
                  TaskHandle_t * const self,
                  uint8_t TimeSlice)
{
    uint32_t *topStack = NULL;
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t));
    *self = ( TCB_t *) NewTcb;
    NewTcb->TimeSlice = TimeSlice;
    NewTcb->uxPriority = uxPriority;
    NewTcb->pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) alignment_byte)));
    NewTcb->pxTopOfStack = pxPortInitialiseStack(topStack,pxTaskCode,pvParameters,self);
    schedule_currentTCB = NewTcb;

    TaskListAdd(NewTcb, Ready);
    StateSet(NewTcb, Ready);
}




void ListDelayInit(void)
{
    ListInit( &OneDelayList );
    ListInit( &TwoDelayList);
    WakeTicksList = &OneDelayList;
    OverWakeTicksList = &TwoDelayList;
}


//Task handle can be hide, but in order to debug, it must be created manually by the user
TaskHandle_t leisureTcb = NULL;

void leisureTask( void )
{//leisureTask content can be manually modified as needed
    while (1) {

    }
}

void LeisureTaskCreat(void)
{
    xTaskCreate(    (TaskFunction_t)leisureTask,
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


Class(SysTicks){
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
}


void HandlerInit(void)
{
    PendSvInit();
    SysTickInit();
}

__attribute__( ( always_inline ) )  inline void SchedulerStart( void )
{
    HandlerInit();
    StartFirstTask();
}


void DelayListRemove(TaskHandle_t self)
{
    ListRemove(WakeTicksList, &(self->task_node));
}



void CheckTicks(void)
{
    uint32_t UpdateTickCount;
    ListNode *list_node,*next_node,*tail_node;

    UpdateTickCount = NowTickCount + 1;
    NowTickCount = UpdateTickCount;

    if( UpdateTickCount == ( uint32_t) 0UL) {
        TheList *temp;
        temp = WakeTicksList;
        WakeTicksList = OverWakeTicksList;
        OverWakeTicksList = temp;
    }

    list_node = WakeTicksList->head;
    if(  list_node == NULL ) {
        schedule();
        return;
    }

    while( list_node->value <= UpdateTickCount ) {
        next_node = list_node->next;
        tail_node = WakeTicksList->tail;

        TaskHandle_t self = container_of(list_node, TCB_t, task_node);
        DelayListRemove(self);
        TaskListAdd(self, Ready);

        if (list_node != tail_node) {
            list_node = next_node;
        } else {
            break;
        }
    }
    schedule();
}




void SysTick_Handler(void)
{
    uint32_t xre = xEnterCritical();
    // If the idle task is suspended, the scheduler is suspended
    if(CheckTaskState(leisureTcb, Suspend) == 0) {
        CheckTicks();
    }
    xExitCritical(xre);
}



