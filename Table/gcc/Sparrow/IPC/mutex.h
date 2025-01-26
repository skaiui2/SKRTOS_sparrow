#ifndef STM32F1_MUTEX_H
#define STM32F1_MUTEX_H

#include <stdint.h>

/* If you use a mutex, you must not set a task's priority to exactly 1 more than
 * the maximum priority of tasks that may be blocked by the mutex. Because Sparrow
 * does not support tasks with the same priority, when priority inversion occurs,
 * the task's priority will be set to the priority of the highest-priority blocking task + 1.
 */

typedef struct Mutex_struct *Mutex_Handle;

Mutex_Handle mutex_creat(void);
void mutex_delete(Mutex_Handle mutex);
uint8_t mutex_lock(Mutex_Handle mutex,uint32_t Ticks);
uint8_t mutex_unlock( Mutex_Handle mutex);


#endif //STM32F1_MUTEX_H
