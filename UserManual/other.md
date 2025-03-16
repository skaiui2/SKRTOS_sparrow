# 使用手册

大部分功能与数表版本相同，基础功能请参考数表版本。

## 链表版本

链表版本内核是在数表版本内核上的拓展，增加功能如下：

### 链表

提供基本的加入和删除链表节点功能

```

void ListInit(TheList *xList);
void ListNodeInit(ListNode *node);
void ListAdd(TheList *xList, ListNode *new_node);
void ListRemove(TheList *xList, ListNode *rm_node);
```

创建的链表为环形链表。



### 时间片

可自定义时间片，设置时间片的周期是1ms，时间片用完后再会轮转到下一个任务。时间片只针对同优先级任务有效

```

void xTaskCreate( TaskFunction_t pxTaskCode,
                  const uint16_t usStackDepth,
                  void * const pvParameters,
                  uint32_t uxPriority,//可设置同优先级
                  TaskHandle_t * const self,
                  uint8_t TimeSlice)//可设置不同时间片

```

### 互斥锁

正常使用互斥锁即可，发生优先级反转时持有锁的任务会暂时提升优先级至阻塞任务中最大优先级。



## 红黑树版本内核

红黑树版本支持同优先级，但不支持时间片功能。

**同优先级支持抢占**

两个任务配置为相同优先级时，后加入就绪树的任务会获得CPU执行权，如果没有同优先级或更高优先级的任务变为就绪态，该任务会一直执行。

**其他功能与链表版本相同。**

调度算法使用缓存指针设计，具有O(1)时间复杂度。

## 响应EDF版本

在传统EDF算法的基础上添加了双阈值，使用低通滤波计算任务平滑执行时间，其中加权指数设置为对历史数据敏感。

### 创建任务

```
void taskA()
{
    while (1) {
        uint32 time1 = TaskEnter();//返回进入时间，但只返回绝对时钟的低32位
        任务内容
        uint32_t time2 = TaskExit();//返回退出时间，但只返回绝对时钟的低32位，并阻塞任务直到下一个周期
    }
}


void APP()
{
	 TaskCreate(    taskA,//任务函数名称
                    256,//任务栈大小
                    NULL,//任务函数参数
                    20,//周期，每20ms执行一次
                    1,//响应阈值
                    100,//截止阈值，任务执行超过阈值视为失败
                    &tcb1//参数
    );
}



int main(void)
{
    初始化
    SchedulerInit();
    APP();
    SchedulerStart();

}

```

### 平滑时间估计

TCB_t结构体中SmoothTime字段将会计算任务的平滑执行时间，读者在调试时打印即可查看，为保证时间精度，**实际时间为SmoothTime字段右移16位得到的值。**

平滑执行时间对历史数据敏感，反映长期执行时间，这是因为在计算时，历史执行数据的权重远大于当前数据权重。

## 结语

总功能如上，目前的Sparrow RTOS只是一个多任务调度内核，并不具有丰富的服务，不过读者可以根据需要添加或修改某些功能。

如果读者在使用过程中发现了某些bug，或者有某些建议，欢迎反馈！







