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


#ifndef SCHEDULE_H
#define SCHEDULE_H


#include <stddef.h>
#include <stdint.h>
#include "rbtree.h"

#define true    1
#define false   0

//if a task is Dead, the leisure task will delete it,this is task delete logic.

#define Ready       0
#define Suspend     1
#define Block       2
#define Dead        3
#define Delay       4
#define BlockDelay  5  //task wait incident happen within a certain time frame.
#define StateLess   6


#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )

#define alignment_byte               0x07
#define config_heap   (14*1024)
#define configMaxPriority 32
#define configShieldInterPriority 191



void TaskDelay( uint16_t ticks );

typedef void (* TaskFunction_t)( void * );
typedef  struct TCB_t         *TaskHandle_t;

void xTaskCreate( TaskFunction_t pxTaskCode,
                  uint16_t usStackDepth,
                  void *pvParameters,
                  uint32_t uxPriority,
                  TaskHandle_t *self );
void xTaskDelete(TaskHandle_t self);

uint8_t TaskPrioritySet(TaskHandle_t taskHandle,uint8_t priority);

void TaskTreeAdd(TaskHandle_t self, uint8_t State);
void TaskTreeRemove(TaskHandle_t self, uint8_t State);
void DelayTreeRemove(TaskHandle_t self);
void Insert_IPC(TaskHandle_t self, rb_root *root);
void Remove_IPC(TaskHandle_t self);

void schedule( void );
void SchedulerInit( void );
void SchedulerStart( void );

void StateSet( TaskHandle_t taskHandle,uint8_t State);
uint8_t CheckIPCState( TaskHandle_t taskHandle);
uint8_t CheckTaskState( TaskHandle_t taskHandle, uint8_t State);

TaskHandle_t GetCurrentTCB(void);
TaskHandle_t TaskHighestPriority(rb_root_handle root);
TaskHandle_t IPCHighestPriorityTask(rb_root_handle root);
uint8_t GetTaskPriority(TaskHandle_t taskHandle);




#endif
