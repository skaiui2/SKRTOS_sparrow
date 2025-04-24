# User Manual

Most functionalities are similar to the Table Version. For basic features, please refer to the Table Version documentation.

## Linked List Version

The Linked List Version kernel is an extension of the Table Version, with the following additional features:

### Linked List

Provides basic functionality for adding and removing nodes in a circular linked list.

```
void ListInit(TheList *xList);
void ListNodeInit(ListNode *node);
void ListAdd(TheList *xList, ListNode *new_node);
void ListRemove(TheList *xList, ListNode *rm_node);
```

The created linked list operates as a circular linked list.

### Time Slicing

Customizable time slices, with each time slice set to a period of **1ms**. After the time slice expires, the next task in the same priority group is scheduled. Time slicing applies only to tasks with the **same priority**.

```
void xTaskCreate(TaskFunction_t pxTaskCode,
                 const uint16_t usStackDepth,
                 void * const pvParameters,
                 uint32_t uxPriority,    // Can set identical priorities
                 TaskHandle_t * const self,
                 uint8_t TimeSlice);     // Allows different time slices
```

### Mutex

Standard mutex usage applies. In case of **priority inversion**, the task holding the mutex temporarily elevates its priority to match the highest priority of the blocking tasks.

## Red-Black Tree Version Kernel

The Red-Black Tree Version supports tasks with the **same priority** but does **not** support time slicing.

### **Preemption with Same Priority**

When two tasks are assigned the same priority:

- The task **most recently added** to the ready tree gets CPU execution rights.
- If no other tasks of equal or higher priority become ready, this task will continue executing.

**Other functionalities are identical to the Linked List Version.**

The scheduling algorithm uses a cached pointer design, providing **O(1) time complexity**.

## EDF (Earliest Deadline First) Version

This version enhances the traditional EDF scheduling algorithm by adding **dual thresholds** and using low-pass filtering to calculate smooth task execution times. Weighted indexing ensures the algorithm is sensitive to historical data.

### Task Creation Example

```
void taskA()
{
    while (1) {
        uint32 time1 = TaskEnter(); // Returns entry time (only the lower 32 bits of the absolute clock)
        // Task content
        uint32_t time2 = TaskExit(); // Returns exit time (only the lower 32 bits) and blocks the task until the next period
    }
}

void APP()
{
    TaskCreate(taskA,           // Task function name
               256,             // Task stack size
               NULL,            // Task function parameters
               20,              // Period: executes every 20ms
               1,               // Response threshold
               100,             // Deadline threshold; exceeding it marks the task as failed
               &tcb1);          // Task handle
}

int main(void)
{
    // Initialization
    SchedulerInit();
    APP();
    SchedulerStart();
}
```

### Smooth Time Estimation

The `SmoothTime` field in the `TCB_t` structure calculates the smooth execution time of a task. You can view this by printing it during debugging. To ensure precision, the **actual time is obtained by right-shifting the** `SmoothTime` **field by 16 bits**.

The smooth execution time is **sensitive to historical data**, reflecting long-term execution trends. This is achieved by assigning greater weight to historical execution times compared to current data.

## Conclusion

The functionalities described above summarize the current capabilities of SKRTOS_sparrow. At its core,SKRTOS_sparrow is a **multi-task scheduling kernel** and does not yet provide extensive services. However, users are encouraged to add or modify features to suit their specific needs.

If you encounter any bugs during use or have suggestions for improvement, we welcome your feedback!



