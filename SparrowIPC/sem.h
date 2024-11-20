#ifndef STM32F1_SEM_H
#define STM32F1_SEM_H

#include <stdint.h>


typedef struct Semaphore_struct *Semaphore_Handle;

Semaphore_Handle semaphore_creat(uint8_t value);
void semaphore_delete(Semaphore_Handle semaphore);
uint8_t semaphore_release( Semaphore_Handle semaphore);
uint8_t semaphore_take(Semaphore_Handle semaphore,uint32_t Ticks);




#endif //STM32F1_SEM_H
