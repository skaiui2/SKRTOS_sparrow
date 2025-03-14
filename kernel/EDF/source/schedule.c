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
#include "rbtree.h"
#include "atomic.h"


extern uint32_t *StackInit(uint32_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters);
extern void StartFirstTask(void);
extern void ErrorHandle(void);    
extern uint32_t xEnterCritical();
extern void xExitCritical(uint32_t xre);



rb_root ReadyTree;
rb_root OneDelayTree;
rb_root TwoDelayTree;
rb_root *WakeTicksTree;
rb_root *OverWakeTicksTree;
rb_root SuspendTree;
rb_root DeleteTree;

static volatile uint32_t NowTickCount = ( uint32_t ) 0;
volatile uint64_t AbsoluteClock = 0;

Class(TCB_t)
{
    volatile uint32_t *pxTopOfStack;
    rb_node task_node;
    rb_node IPC_node;
    uint16_t period;
    uint8_t respondLine;
    uint16_t deadline;
    uint32_t EnterTime;
    uint32_t ExitTime;
    uint32_t SmoothTime;
    uint32_t *pxStack;
};

__attribute__( ( used ) )  TaskHandle_t volatile schedule_currentTCB = NULL;

TaskHandle_t GetCurrentTCB(void)
{
    return schedule_currentTCB;
}

uint8_t GetRespondLine(TaskHandle_t self)
{
    return self->respondLine;
}

TaskHandle_t TaskFirstRespond(rb_root_handle root)
{
    rb_node *rb_highest_node = root->first_node;
    
    return container_of(rb_highest_node, TCB_t, task_node);
}

TaskHandle_t FirstRespond_IPC(rb_root_handle root)
{
    rb_node *rb_highest_node = root->first_node;
    return container_of(rb_highest_node, TCB_t, IPC_node);
}

uint8_t SetRespondLine(TaskHandle_t self, uint8_t respondLine)
{
    return (uint8_t)atomic_set_return(respondLine, (uint32_t *)&(self->respondLine));
}



void ReadyTreeAdd(rb_node *node)
{
    TaskHandle_t self = container_of(node, TCB_t, task_node);
    node->value =  AbsoluteClock + self->respondLine;
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
    self->IPC_node.value = AbsoluteClock + self->respondLine;
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



uint8_t CheckIPCState( TaskHandle_t taskHandle)
{
    return taskHandle->IPC_node.root == NULL;
}


uint8_t volatile schedule_PendSV = 0;

void TaskSwitchContext( void )
{
    schedule_PendSV++;
    schedule_currentTCB = TaskFirstRespond(&ReadyTree);
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
    if (ticks) {
        TaskTreeRemove(schedule_currentTCB, Ready);
        RecordWakeTime(ticks);
        schedule();
    }
}

/*
 *  Creat the task, first malloc the stack, then TCB.
 *  For ARM, the address of TCB above the stack.
 */

void  TaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,
                  uint16_t period,
                  uint8_t respondLine,
                  uint16_t deadline,
                  TaskHandle_t * const self
                  )
{
    uint32_t *topStack = NULL;
    uint32_t *pxStack = ( uint32_t *) heap_malloc( ( ( ( size_t ) usStackDepth ) * sizeof( uint32_t * ) ) );
    TCB_t *NewTcb = (TCB_t *)heap_malloc(sizeof(TCB_t));
    *self = ( TCB_t *) NewTcb;
    *NewTcb = (TCB_t){
        .period = period,
        .respondLine = respondLine,
        .deadline = deadline,
        .SmoothTime = 0,
        .pxStack = pxStack
    };
    topStack =  NewTcb->pxStack + (usStackDepth - (uint32_t)1) ;
    topStack = ( uint32_t *) (((uint32_t)topStack) & (~((uint32_t) alignment_byte)));
    NewTcb->pxTopOfStack = StackInit(topStack,pxTaskCode,pvParameters);
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

uint32_t TaskEnter(void)
{
    return schedule_currentTCB->EnterTime = AbsoluteClock;
}



uint32_t TaskExit(void)
{
    TaskHandle_t self = schedule_currentTCB;
    self->ExitTime = AbsoluteClock;
    uint32_t newPeriod = self->ExitTime - self->EnterTime;
    if (self->SmoothTime != 0) {
        self->SmoothTime = (self->SmoothTime - (self->SmoothTime >> 3)) + (newPeriod << 13);
    } else {
        self->SmoothTime = newPeriod << 16;
    }
    if (newPeriod >= self->deadline) {
        ErrorHandle();
    }
    TaskDelay(self->period);
    return newPeriod;
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

/*
 * Task handle can be hide, but in order to debug, it must be created manually by the user.
 * leisureTask content can be manually modified as needed.
 */
TaskHandle_t leisureTcb = NULL;
uint32_t leisureCount = 0;
void leisureTask( void )
{
    while (1) {
        leisureCount++;
        TaskFree();
    }
}

uint64_t MaxRespondLine = (uint64_t)~0;
void LeisureTaskCreat(void)
{
    TaskCreate(     (TaskFunction_t)leisureTask,
                    128,
                    NULL,
                    0,
                    MaxRespondLine,
                    MaxRespondLine,
                    &leisureTcb );
    leisureTcb->task_node.value = MaxRespondLine;
}


void SchedulerInit(void)
{
    ADTTreeInit();
    TreeDelayInit();
    LeisureTaskCreat();
}


void SchedulerStart( void )
{
    
    TaskSwitchContext();
    StartFirstTask();
}


void DelayTreeRemove(TaskHandle_t self)
{
    rb_remove_node(WakeTicksTree, &(self->task_node));
}


uint8_t SusPend = 1;
void CheckTicks(void)
{
    rb_node *rb_node = NULL;

    AbsoluteClock++;
    NowTickCount++;
    if (SusPend) {
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
          if (self->task_node.value <= schedule_currentTCB->task_node.value) {
       
            schedule();
          }
      }
    
    }
}



uint32_t xEnterCritical()
{
  uint32_t lock = SusPend; 
  SusPend = 0;
  return lock;
}

void xExitCritical(uint32_t xre)
{
  SusPend = xre;
}



