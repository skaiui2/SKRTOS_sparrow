//
// Created by el on 2024/11/9.
//

#ifndef STM32F1_SCHEDULE_H
#define STM32F1_SCHEDULE_H

#include <stddef.h>
#include <stdint-gcc.h>

#define true    1
#define false   0

#define Ready       1
#define Delay       2
#define Suspend     3
#define Block       4

//config
#define IPC    0
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )

#define alignment_byte               0x07
#define config_heap   (8*1024)
#define configMaxPriori 32
#define configShieldInterPriority 191


#define Class(class)    \
typedef struct  class  class;\
struct class



uint32_t  xEnterCritical( void );
void xExitCritical( uint32_t xReturn );

uint8_t FindHighestPriority( uint32_t Table );

void TaskDelay( uint16_t ticks );

typedef void (* TaskFunction_t)( void * );
typedef  struct TCB_t         *TaskHandle_t;

void xTaskCreate( TaskFunction_t pxTaskCode,
                  uint16_t usStackDepth,
                  void *pvParameters,
                  uint32_t uxPriority,
                  TaskHandle_t *self );

void schedule( void );
void SchedulerInit( void );
void SchedulerStart( void );

uint32_t StateAdd( TaskHandle_t taskHandle,uint8_t State);
uint32_t StateRemove( TaskHandle_t taskHandle, uint8_t State);
uint8_t CheckState( TaskHandle_t taskHandle,uint8_t State );

TaskHandle_t GetTaskHandle( uint8_t i);
uint8_t GetTaskPriority( TaskHandle_t taskHandle);



#endif //STM32F1_SCHEDULE_H
