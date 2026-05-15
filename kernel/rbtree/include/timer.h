

#ifndef TIMER_H
#define TIMER_H

#define run 1
#define stop 0
#include "schedule.h"


typedef void (* TimerFunction_t)( void * );
typedef struct timer_struct * TimerHandle;
TaskHandle_t TimerInit(uint8_t timer_priority, uint16_t stack, uint8_t check_period);
TimerHandle TimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t timer_flag);
uint8_t TimerRerun(TimerHandle timer, uint8_t timer_flag);
uint8_t TimerStop(TimerHandle timer);
uint8_t TimerStopImmediate(TimerHandle timer);

#endif
