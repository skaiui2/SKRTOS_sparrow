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


#include "class.h"

#define true    1
#define false   0

#define Dead        0
#define Ready       1
#define Delay       2
#define Suspend     3
#define Block       4

//config
#define alignment_byte               0x07
#define config_heap   (10240)
#define configMaxPriority 32
#define configTimerNumber  32



typedef void (* TaskFunction_t)( void * );
typedef  struct TCB_t         *TaskHandle_t;

uint8_t FindHighestPriority(uint32_t Table);

uint32_t TableAdd( TaskHandle_t taskHandle,uint8_t State);
uint32_t TableRemove( TaskHandle_t taskHandle, uint8_t State);

void TaskDelay( uint16_t ticks );
void TaskCreate(  TaskFunction_t pxTaskCode,
                  uint16_t usStackDepth,
                  void *pvParameters,
                  uint32_t uxPriority,
                  TaskHandle_t *self );
void TaskDelete(TaskHandle_t self);
void TaskSusPend(TaskHandle_t self);
void TaskResume(TaskHandle_t self);

void SchedulerInit( void );
void SchedulerStart( void );
void SchedulerSusPend(void);
void SchedulerResume(void);

void CheckTicks(void);
uint8_t CheckState( TaskHandle_t taskHandle,uint8_t State );

TaskHandle_t GetTaskHandle( uint8_t i);
uint8_t GetTaskPriority( TaskHandle_t taskHandle);
TaskHandle_t GetCurrentTCB(void);

void PreemptiveCPU(uint8_t priority);



#endif
