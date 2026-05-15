# Table Version User Manual

## Porting

Locate the `kernel\table` folder in the `main` branch, and then navigate to the `arch\arm` folder corresponding to your development board architecture. Choose the appropriate interface files based on your requirements.

Add these files to your original project.

For **ARM CM3 architecture**, for example with **STM32F1**, remove the `SVC_Handler`, `SysTick_Handler`, and `PendSV_Handler` interrupts from the `stm32f1xx_it.c` file to successfully complete the porting process.

For **ARM CA7 architecture**, only add the port files.

## Supported Features

- **Memory**: Supports small memory management and memory pools.
- **Tasks**: Includes task readiness, suspension, delay, and deletion functionalities.
- **Scheduler**: Implements priority-based preemptive scheduling, with support for scheduler suspension.
- **IPC Mechanism**: Provides message queues, semaphores, mutexes, buffer queues, and read-write locks.
- **Additional Features**: Includes timers, atomic operations, and critical sections.

### **Configuration**

Configuration options are located in `schedule.h` and the `config` files within the `port` folder.

#### **schedule.h**

- `configMaxPriority`: Specifies the maximum priority for tasks and the number of supported tasks.
- `alignment_byte`: Specifies memory management byte alignment. Default is **8-byte alignment**. To modify, set it to `alignment_value - 1`.
- `config_heap`: Defines the size of the task stack, which can be customized as needed.
- `configTimerNumber`: Determines the number of timers.

Example configuration:

```
//config
#define alignment_byte               0x07
#define config_heap   (10240)
#define configMaxPriority 32
#define configTimerNumber  32
```

#### **config.h**

- `configSysTickClockHz`: Specifies the SysTick clock frequency. The default value is **72 MHz**.
- `configTickRateHz`: Specifies the interrupt frequency. By default, `1000` represents **1ms per interrupt**.
- `configShieldInterPriority`: Sets the maximum interrupt priority masked during critical sections. Since the upper 4 bits are valid, the default value is **11**.

Example configuration:

```
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )
#define configShieldInterPriority 191
```





## Recommended Implementation

Using **taskA** as an example, you can add additional tasks based on this structure.

```
TaskHandle_t tcbTask1 = NULL; // Manually define the handle

void taskA()
{
    while (1){
        // Task logic
    }
}

void APP()
{
    TaskCreate(     taskA,         // Task function to be executed
                    128,           // Configure based on variables and nesting levels; actual size is this 										// value * 4 bytes
                    NULL,          // Parameters (default is NULL)
                    1,             // Priority
                    &tcbTask1      // Handle, needs to be manually defined
    );
}

int main(void)
{
    // BSP initialization
    SchedulerInit(); // Initialize the scheduler
    APP();           // Create the task
    SchedulerStart();// Start the scheduler

    // Normally, execution will not reach here
    while (1)
    {

    }
}

```



## Using APIs (Task Management)

### Task States

In the kernel, tasks can be in one of six states: **Running**, **Ready**, **Delayed**, **Blocked**, **Suspended**, and **Dead**.

- **Running State**: The task is currently executing. There can only be one task in the running state at any given time. When a task is running, it is also considered ready.
- **Ready State**: The task is prepared to run and may be running if it has the highest priority. **Only tasks in the ready state are eligible for execution**.
- **Delayed State**: The task is in a delay mode. A task in the delayed state can also be in the blocked state, for example, when waiting for an event. If the event does not occur within the maximum wait time (e.g., 100ms), the task finishes its delay period. If the event occurs during this time, the task executes immediately. In this case, the task is both delayed and blocked.
- **Blocked State**: The task is waiting for an event to occur. It can also be simultaneously in the delayed state.
- **Suspended State**: If a task does not need urgent execution for an extended period, it can be suspended. Additionally, the scheduler itself can be suspended. This state is typically set manually depending on the user's task management requirements.
- **Dead State**: The task is marked as dead and will be cleaned up by the idle task.

### Task Creation

```
void TaskCreate( TaskFunction_t pxTaskCode,  /* The task function to be executed. Add typecast if passing arguments. */             
                 uint16_t usStackDepth,     /* Configure based on task variables and nesting levels; actual size is this value * 4 bytes. */
                 void *pvParameters,        /* Parameters (default is NULL). */
                 uint32_t uxPriority,       /* Priority; note that tasks cannot share the same priority. */
                 TaskHandle_t *self );      /* Handle, which must be manually defined. */

```



like:

```
TaskCreate(        taskA,
                   128,
                   NULL,
                   1,
                   &tcbTask1
);

```

### Task Delay

```
void TaskDelay(uint16_t ticks);
```

Add this function within a task to implement delay, where the unit of `ticks` is **milliseconds**.

### Task Suspension

```
void TaskSusPend(TaskHandle_t self);
void TaskResume(TaskHandle_t self);
```

You can suspend a specific task, preventing it from being executed until it is resumed back to the ready state.

**Example:**

```
extern TaskHandle_t leisureTcb;
void taskA() {
    while (1) {
        TaskSusPend(leisureTask);
    }
}
```

#### Scheduler Suspension

```
void SchedulerSusPend(void);
```

Suspending the scheduler prevents task switching.

### Task Deletion

```
void TaskDelete(TaskHandle_t self);
```

**Example Usage:**

```
TaskHandle_t tcbTask1 = NULL;
TaskHandle_t tcbTask3 = NULL;

void taskA() {
    while (1) {
        // Task logic
    }
}

void APP() {
    TaskCreate(...);
    TaskDelete(tcbTask3); // Delete the created task
}

int main(void) {
    // Initialization code omitted
    TaskCreate((TaskFunction_t)APP,
               128,
               NULL,
               1,
               &tcbTask3,
               0);
    SchedulerStart();
}
```

### Critical Section

Critical sections can be added before and after code segments where concurrency issues need to be avoided. Nested usage is supported.

```
uint32_t xEnterCritical(void);
void xExitCritical(uint32_t xReturn);
```

**Example:**

```
uint32_t xReturn = xEnterCritical();

a = 1; // A shared variable modified by multiple tasks      
        
xExitCritical(xReturn);
```

### Memory Management

```
void *heap_malloc(size_t WantSize);
void heap_free(void *xReturn);
```

These functions are similar to C language's `malloc` and `free`, but the address returned by `heap_malloc` is **8-byte aligned** by default. The size of memory blocks allocated is also **8-byte aligned**, which can be modified in the configuration.

#### **Note**

Memory is inherently a shared resource. However, `heap_malloc` and `heap_free` do not provide concurrency protection internally. When dynamically allocating memory during task execution, race conditions may occur, leading to program errors. To prevent this, always protect memory operations with a **critical section**.

**Example:**

```
void taskA(void)
{
    while (1) {
        uint32_t xReturn = xEnterCritical();
        void *a = heap_malloc(sizeof(int));
        xExitCritical(xReturn);
        
        a = 1; // Utilize allocated memory
        
        xReturn = xEnterCritical();
        heap_free(a);
        xExitCritical(xReturn);
    }
}
```

You can also encapsulate memory management APIs for convenience:

```
void *os_malloc(size_t WantSize)
{
    uint32_t xReturn = xEnterCritical();
    void *a = heap_malloc(WantSize);
    xExitCritical(xReturn);
    return a;
}

void os_free(void *ptr)
{
    uint32_t xReturn = xEnterCritical();
    heap_free(ptr);
    xExitCritical(xReturn);
}
```

This allows direct usage of memory management functions in tasks.

**Alternative Method**

Race conditions arise from concurrency. To avoid issues, you can also **suspend the scheduler** during memory allocation or release operations. While the scheduler is suspended, other tasks cannot preempt, effectively avoiding risks caused by competition.

### Memory Pool

The complete API for memory pools is as follows:

```
PoolHeadHandle memPool_creat(uint16_t size, uint8_t amount);
void *memPool_apl(PoolHeadHandle ThePool);
void memPool_free(void *xRet);
void memPool_delete(PoolHeadHandle ThePool);
```

**Recommended Usage:**

```
void taskA()
{
    while (1) {
        int *a = memPool_apl(pool); // Obtain a memory block from the pool
        *a = 1;
        memPool_free(a); // Release the memory block back to the pool
    }
}

void APP()
{
    PoolHeadHandle pool = memPool_creat(80, 10); // Configure memory pool size and block count
    // Task creation and other logic
    TaskCreate(......);
}
```



## Using APIs (Task Communication)

### Message Queue

```
Queue_Handle queue_creat(uint32_t queue_length, uint32_t queue_size); // Define queue length and message size
void queue_delete(Queue_Handle queue);                               // Delete the queue
uint8_t queue_send(Queue_Handle queue, uint32_t *buf, uint32_t Ticks); // Send a message; Ticks specifies max wait time (ms)
uint8_t queue_receive(Queue_Handle queue, uint32_t *buf, uint32_t Ticks); // Receive a message; Ticks specifies max wait time (ms)
```

These APIs are used to create, delete, send, and receive messages from a queue.

### **Basic Mechanism**

- **Message Sending**:
  - Messages can be sent in one-to-one or many-to-many configurations.
  - If the queue is not full, the message is written normally.
  - If the queue is full, the sender waits for the specified time (Ticks). If the time expires before the message can be written, the operation exits and returns **false**.
- **Message Receiving**:
  - If there are messages in the queue, the receiver reads them normally.
  - If the queue is empty, the receiver waits for the specified time (Ticks). If the time expires before a message becomes available, the operation exits and returns **false**.

### **Recommended Usage**

```
struct AMessage
{
    char ucMessageID;
    char ucData[20];
};
struct AMessage xMessage;
struct AMessage aMessage;

// Define handle for receiving messages
Queue_Handle xStructQueue = NULL;

void vCreateQueues(void)
{
    xMessage.ucMessageID = 0xab;
    aMessage.ucMessageID = 0x01;
    memset(&(xMessage.ucData), 0x12, 20);
    xStructQueue = queue_creat(1, sizeof(xMessage));
}

// Task Area: The user must manually create task handles for debugging and specification
TaskHandle_t tcbTask1 = NULL;
TaskHandle_t tcbTask2 = NULL;

void taskA()
{
    while (1) {
        queue_send(xStructQueue, (void *)&xMessage, 1);
        TaskDelay(1);
    }
}

void taskB()
{
    struct AMessage xRxedStructure;
    while (1) {
        queue_receive(xStructQueue, (void *)&xRxedStructure, 1);
    }
}

void APP()
{
    xStructQueue = vCreateQueues(); // Create message queue
    
    // Create tasks: taskA, taskB
}

int main(void)
{
    // BSP initialization
    SchedulerInit();
    APP();
    SchedulerStart();
}
```

### **Interrupt Usage Considerations**

- Only task objects can use delay-wait mechanisms. Interrupts are **not considered tasks**, so tick wait times cannot be set.
- When using the queue in an interrupt, always set **Ticks to 0**.

### Semaphore

```
Semaphore_Handle semaphore_creat(uint8_t value);         // Create a semaphore
void semaphore_delete(Semaphore_Handle semaphore);       // Delete a semaphore
uint8_t semaphore_release(Semaphore_Handle semaphore);   // Release a semaphore
uint8_t semaphore_take(Semaphore_Handle semaphore, uint32_t Ticks); // Take a semaphore, with Ticks as the max wait time (in ms)
```

`semaphore_creat(uint8_t value)` is the function to create a semaphore, where the `value` determines its initial state. It can be set based on specific requirements.

#### **Different Initialization Values and Their Functions**

Semaphores can be used for two primary purposes: **mutual exclusion** or **conditional synchronization**. The functionality depends on the initial value set during creation.

**Initialization > 1: Counting Semaphore**

If initialized with a value greater than `1`, the semaphore acts as a **counting semaphore**. For example, with 5 resources available and 6 tasks attempting access, the 6th task will block until a resource becomes available or the maximum wait time (e.g., 10ms) is reached.

c

```
void taskA()
{
    while(1){
        semaphore_take(semaphore, 10); // Lock
        // Code to access resources
        semaphore_release(semaphore); // Unlock
    }
}

void APP()
{
    semaphore = semaphore_creat(5); // Semaphore initialized to 5 resources
    // Create tasks here
}
```

**Initialization = 1: Mutual Exclusion**

Semaphores, such as **mutexes or spinlocks**, are commonly used for **exclusive access** to resources. Here, the `value` is set to the number of resources (for single access).

```
void taskA()
{
    while(1){
        semaphore_take(semaphore, 10); // Lock
        // Code to access the resource
        semaphore_release(semaphore); // Unlock
    }
}

void APP()
{
    semaphore = semaphore_creat(1); // Semaphore initialized to 1
    // Create tasks here
}
```

**Priority Inversion**: In some cases, semaphores may cause priority inversion. Proper task priority allocation can mitigate this issue. Alternatively, using mutexes can be an effective solution.

**Note**: Any case where the semaphore is initialized to a value other than `0` can be considered **mutual exclusion** for exclusive resource access.

**Initialization = 0: Synchronization**

Semaphores initialized to `0` are often used for **synchronization**. For example, a task will block until a specific condition is met or a signal is sent via the semaphore.

```
void taskA()
{
    // Release the semaphore
    semaphore_release(semaphore);
}

void taskB()
{
    // Block if no semaphore is available
    semaphore_take(semaphore, 10); // Unblocks when the semaphore is available
    // Proceed with execution
}
```

### Mutex

```
Mutex_Handle mutex_creat(void);                          // Create a mutex
void mutex_delete(Mutex_Handle mutex);                   // Delete a mutex
uint8_t mutex_lock(Mutex_Handle mutex, uint32_t Ticks);  // Lock the mutex
uint8_t mutex_unlock(Mutex_Handle mutex);                // Unlock the mutex
```

A mutex is a specialized form of semaphore used for **mutual exclusion**, especially when **priority inversion** might occur. If the maximum wait time is exceeded, the task will unblock and return `false`.

**Example Usage:**

```
void code()
{
    mutex_lock(mutex, 10); // Lock the mutex
    // Code to access the resource
    mutex_unlock(mutex);   // Unlock the mutex
}

void APP()
{
    mutex = mutex_creat(); // Create a mutex
}
```

#### 

### Producer-Consumer Buffer Queue

The producer-consumer buffer queue is designed to address scenarios like:

- Single producer, single consumer
- Multiple producers, single consumer
- Multiple producers, multiple consumers

### **API Overview**

```
// Single Producer, Single Consumer
Oo_buffer_handle Oo_buffer_creat(uint8_t buffer_size);  // Create buffer
void Oo_insert(Oo_buffer_handle Oo_buffer1, int object); // Insert into buffer
int Oo_remove(Oo_buffer_handle Oo_buffer1);             // Remove from buffer
void Oo_buffer_delete(Oo_buffer_handle Oo_buffer1);     // Delete buffer

// Multiple Producers, Single Consumer
Mo_buffer_handle Mo_buffer_creat(uint8_t buffer_size);  // Create buffer
void Mo_insert(Mo_buffer_handle Mo_buffer1, int object); // Insert into buffer
int Mo_remove(Mo_buffer_handle Mo_buffer1);             // Remove from buffer
void Mo_buffer_delete(Mo_buffer_handle Mo_buffer1);     // Delete buffer

// Multiple Producers, Multiple Consumers
Mm_buffer_handle Mm_buffer_creat(uint8_t buffer_size);  // Create buffer
void Mm_insert(Mm_buffer_handle Mm_buffer1, int object); // Insert into buffer
int Mm_remove(Mm_buffer_handle Mm_buffer1);             // Remove from buffer
void Mm_buffer_delete(Mm_buffer_handle Mm_buffer1);     // Delete buffer
```

### **Usage Example**

```
void taskA()
{
    // Use different APIs based on the scenario
    Mm_insert(MrMw_control1, 1); // Insert 1 into the circular buffer
}

void taskB()
{
    int b = Mm_remove(MrMw_control1); // Read from the circular buffer
}

void APP()
{
    MrMw_control1 = Mm_buffer_creat(5); // Create a buffer queue of size 5
}

int main()
{
    // Execute code
}
```

### **Read-Write Lock**

Read-write locks can be applied to scenarios with multiple readers and writers, addressing most resource access issues.

### **API Overview**

```
rwlock_handle rwlock_creat(void);                        // Create lock
void read_acquire(rwlock_handle rwlock_handle1);         // Acquire read lock
void read_release(rwlock_handle rwlock_handle1);         // Release read lock
void write_acquire(rwlock_handle rwlock_handle1);        // Acquire write lock
void write_release(rwlock_handle rwlock_handle1);        // Release write lock
void rwlock_delete(rwlock_handle rwlock1);               // Delete lock
```

### **Usage Example**

```
void taskA()
{
    // Use different APIs for different scenarios
    read_acquire(Oo_buffer1);  // Acquire read lock
    // Perform read operations
    read_release(Oo_buffer1);  // Release lock for other tasks to start reading
}

void taskB()
{
    write_acquire(Oo_buffer1); // Acquire write lock
    // Perform write operations
    write_release(Oo_buffer1); // Release lock for other tasks to start writing
}

void taskC()
{
    read_acquire(Oo_buffer1);  // Acquire read lock
    // Perform read operations
    read_release(Oo_buffer1);  // Release lock for other tasks to start reading
}

void taskD()
{
    write_acquire(Oo_buffer1); // Acquire write lock
    // Perform write operations
    write_release(Oo_buffer1); // Release lock for other tasks to start writing
}

void APP()
{
    rwlock_handle rwlock = rwlock_creat(); // Create read-write lock
}

int main()
{
    // Initialize peripherals
    // Initialize scheduler
    APP(); // Create tasks
    // Start scheduler
```



### Timers

The timer functionality is supported, and all timer callback functions are executed by a single thread. The thread's **priority**, **stack size**, and **check interval** are user-configurable.

### **Timer API Overview**

```
TaskHandle_t TimerInit(uint8_t timer_priority, uint16_t stack, uint8_t check_period);
TimerHandle TimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t index, uint8_t timer_flag);
uint8_t TimerRerun(TimerHandle timer, uint8_t timer_flag);
uint8_t TimerStop(TimerHandle timer);
uint8_t TimerStopImmediate(TimerHandle timer);
void TimerDelete(TimerHandle timer);
```

### **TimerInit Function**

Initializes the timer and creates a timer thread.

- **timer_priority**: Priority of the timer thread.
- **stack**: Stack size of the timer thread.
- **check_period**: Timer check interval in milliseconds. It is recommended to set this as the greatest common divisor (GCD) of the different timer task periods.

### **TimerCreat Function**

Creates and adds a timer to the timer array.

- **CallBackFun**: The callback function to be executed.
- **period**: Timer period.
- **index**: Index in the timer array (maximum array size can be configured in `schedule.h`).
- **timer_flag**:
  - `0` (stop): Timer will execute only once.
  - `1` (run): Timer will execute indefinitely by default.

### **Other Timer Functions**

- **TimerRerun**: Re-adds the timer to the list and restarts it. You can configure the flag to run once or indefinitely.
- **TimerStop**: Removes the timer from the list after it runs one more time.
- **TimerStopImmediate**: Immediately removes the timer from the list.
- **TimerDelete**: Deletes the timer entirely.

### **Usage Example**

```
int a = 0;
void count(void)
{
    a++;
}

void APP()
{
    TaskHandle_t tcbTask3 = TimerInit(4, 128, 1); // Set timer priority, stack size, and check interval
    TaskCreate((TaskFunction_t)taskA, 128, NULL, 3, &tcbTask1, 0);
    TaskCreate((TaskFunction_t)taskB, 128, NULL, 2, &tcbTask2, 0);
    TimerHandle timerHandle = TimerCreat((TimerFunction_t)count, 1, 0, stop); // Create a timer
}
```

### Atomic Operations

```
void atomic_add(uint32_t i, uint32_t *v);          // Non-returning version, `i` is the operation value, `v` is the target address
int atomic_add_return(uint32_t i, uint32_t *v);   // Adds and returns the result

void atomic_sub(uint32_t i, uint32_t *v);         
int atomic_sub_return(uint32_t i, uint32_t *v);   // Subtracts and returns the result

atomic_inc_return(v); // Increment and return
atomic_dec_return(v); // Decrement and return

uint32_t atomic_set_return(uint32_t i, uint32_t *v); // Set the value of the target and return
uint32_t atomic_set(uint32_t i, uint32_t *v);
```

### **Important Note**

Ensure that the target variable address is **byte-aligned**.

## Conclusion

The functions outlined above summarize the capabilities of Sparrow RTOS. Currently, Sparrow RTOS is a lightweight multi-task scheduling kernel and does not include extensive service features. However, users are encouraged to add or modify features based on their specific needs.

If you encounter any bugs during use or have suggestions for improvement, feedback is always welcome!

