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

#ifndef TIMER_H
#define TIMER_H

#define run 1
#define stop 0
#include "schedule.h"


typedef void (* TimerFunction_t)( void * );
typedef struct timer_struct * TimerHandle;

TaskHandle_t TimerInit(uint16_t stack, uint16_t period, uint8_t RespondLine, uint32_t deadline, uint8_t check_period);
TimerHandle TimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t timer_flag);
uint8_t TimerRerun(TimerHandle timer, uint8_t timer_flag);
uint8_t TimerStop(TimerHandle timer);
uint8_t TimerStopImmediate(TimerHandle timer);

#endif
