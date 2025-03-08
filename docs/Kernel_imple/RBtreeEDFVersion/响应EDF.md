## 调度器

该版本内核并没有采用传统的固定优先级，而是采用类似EDF算法的实现。



## 调度算法

笔者也不清楚这叫什么算法，或许应该算双重阈值？

算了，无所谓，就叫它响应EDF算法好了。



### 思路

传统的EDF算法在加入就绪队列时，排序的标准是根据截止时间的大小进行排序，不过笔者认为有些场景下或许不妥：

当执行的任务非常重要，但又耗费时间较多。

由于任务耗费时间较多，我们应当考虑适当提高截止时间，但是提高截止时间，又会导致任务的重要性下降。

所以笔者加入了响应阈值作为调度标准。

当然，这个算法目前就是个玩具，只是笔者的尝试。

考虑到需要对该算法进行优化，因此笔者加入了平滑执行时间的计算，后期会考虑使用弹性响应阈值，根据平滑执行时间动态调整响应阈值的范围。

如果后期进行弹性优化，那么会考虑参考TCP/IP协议栈中的RTT估计器进行动态预测。

### 平滑执行时间

计算使用的算法是指数加权移动平均，平滑执行时间=7/8 历史数据 + 1/8 目前数据。



## 代码实现

### 结构体

如下：

```
Class(TCB_t)
{
    volatile uint32_t *pxTopOfStack;
    rb_node task_node;
    rb_node IPC_node;
    uint16_t period;
    uint8_t respondLine;
    uint16_t deadline;
    uint32_t EnterTime;
    uint32_t ExitTime;
    uint32_t SmoothTime;
    uint32_t *pxStack;
};
```

字段：

period：任务周期

respondLine：响应阈值

deadline：截止阈值

EnterTime：任务开始的时间

ExitTime：任务结束的时间

SmoothTime：任务平滑执行时间

### 平滑执行时间

为了保证SmoothTime的精度，将计算数进行右移，从而避免了浮点数，得到一个近似的结果。

```
uint32_t TaskExit(void)
{
    TaskHandle_t self = schedule_currentTCB;
    atomic_set((uint32_t)AbsoluteClock, &(self->ExitTime));
    uint32_t newPeriod = self->ExitTime - self->EnterTime;
    if (self->SmoothTime != 0) {
        self->SmoothTime = (self->SmoothTime - (self->SmoothTime >> 3)) + (newPeriod << 13);
    } else {
        self->SmoothTime = newPeriod << 16;
    }
    if (newPeriod >= self->deadline) {
        ErrorHandle();
    }
    TaskDelay(self->period);
    return newPeriod;
}

```

