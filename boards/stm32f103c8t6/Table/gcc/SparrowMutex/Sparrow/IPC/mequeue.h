//
// Created by el on 2024/11/19.
//

#ifndef STM32F1_MEQUEUE_H
#define STM32F1_MEQUEUE_H
#include "schedule.h"

typedef struct Queue_struct *Queue_Handle;

Queue_Handle queue_creat(uint32_t queue_length,uint32_t queue_size);
void queue_delete( Queue_Handle queue );
uint8_t queue_send(Queue_Handle queue, uint32_t *buf, uint32_t Ticks);
uint8_t queue_receive( Queue_Handle queue, uint32_t *buf, uint32_t Ticks );


#endif //STM32F1_MEQUEUE_H
