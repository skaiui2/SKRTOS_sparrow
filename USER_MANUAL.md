# Sparrow RTOS使用手册

## 简介

Sparrow RTOS是一个极简型RTOS内核，定位是学习型RTOS。

项目地址：[skaiui2/SKRTOS_sparrow: Lightweight rtos inspired by SKRTOS](https://github.com/skaiui2/SKRTOS_sparrow)

在项目下有五个分支:

main：不同平台的已移植工程，例如：keil、IAR、clion（gcc）

experiment：实验工程，需要搭配教程。

manual：arm cm3和arm cm4的手册。

memory：内存管理相关的实验工程及单独的内存管理函数库。

**source**：Sparrow RTOS的源码，主要维护和更新该分支。



教程地址：[400行程序写一个实时操作系统（开篇） - 知乎](https://zhuanlan.zhihu.com/p/963319443)

该教程的代码为数表版本内核，教程不仅讲解原理，也指导读者如何一步步完成一个RTOS内核。



## 源码

Sparrow RTOS共有三个版本：数表版本(Table)、链表版本(List)、红黑树版本(RBtree)。

**教程讲解的版本为数表版本**

也可以把这三个版本划分为：学习版、使用版、实验版

三个版本中支持的编译器和平台正在维护更新中，目前默认支持的架构是arm cm3，默认支持的编译器为gcc。

数表版本只支持32个以下的任务，且不支持同优先级，该版本支持keil、gcc、IAR多种编译器，其中不带IPC版本仅400行代码。

链表版本支持同优先级，对支持的任务数量没有限制，该版本接口部分可参考数表版本进行修改。

红黑树版本支持同优先级，但不支持时间片，其他功能与链表版本相同。

以上三个版本的设计思想并没有任何区别。

**适用范围**

数表版本：学习用途或资源有限的条件。

链表版本：完成度较高，适合大多数场景。

红黑树版本：适用于大量任务频繁搜索、插入和删除操作的场景。



## 移植

将其添加到原工程的文件中，并且删除掉stm32f1xx_it.c文件中的SVC_Handler、SysTick_Handler和PendSV_Handler中断，即可成功完成移植。

### 不同平台的移植问题

Sparraw RTOS目前只适用于arm cm3架构，考虑到不同平台的移植问题，在设计时，笔者将Sparrow RTOS的接口定义与FreeRTOS内核接口一致，如果要移植到不同架构和平台，可以直接参考FreeRTOS的实现。

移植接口主要是内联汇编部分，由于不同平台对内联汇编的支持不一样，这部分汇编代码需要读者结合具体的开发环境进行修改。

#### 移植时架构接口：

vPortSVCHandler、xPortPendSVHandler、pxPortInitialiseStack。这三个接口与具体架构有关，可以参考FreeRTOS相关架构的实现进行修改。

#### 临界区接口：

xEnterCritical、 xExitCritical临界区函数也与具体架构的中断屏蔽寄存器有关，也可以参考一些开源的RTOS代码进行修改。

#### 汇编接口：

FindHighestPriority函数使用clz指令寻找最高优先级任务，可以结合具体编译器进行修改，也可以修改为C语言版本，例如：

**gcc版本**

```
__attribute__((always_inline)) inline uint8_t FindHighestPriority(uint32_t Table) {
    return 31 - __builtin_clz(Table);
}
```

**C语言版本**

```
__attribute__((always_inline)) inline uint8_t FindHighestPriority(uint32_t Table) {
    uint8_t TopZeroNumber = 0;
    while ((Table & (1 << (31 - TopZeroNumber))) == 0) {
        TopZeroNumber++;
    }
    return 31 - TopZeroNumber;
}
```

### 移植注意

在keil环境下，当移植完Sparrow RTOS后，如果编译没报错，但是程序不能运行，请尝试降低keil的编译器优化等级。这是因为cubemax生成的工程默认编译优化全开，可能导致出现bug。



## 推荐写法

以taskA为例，多任务在此基础上添加即可

```

TaskHandle_t tcbTask1 = NULL;//手动定义句柄

void taskA()
{
    while (1){
        //任务代码
    }
}


void APP( )
{
    xTaskCreate(    taskA,//需要执行的任务函数
                    128,//根据任务中的变量、嵌套层数进行配置，真实大小为该值*4个字节
                    NULL,//形参，默认为NULL
                    1,//优先级
                    &tcbTask1//句柄，需要手动定义
    );
    

}


int main(void)
{
    //bsp初始化
    SchedulerInit();//初始化调度器
    APP();//创建任务
    SchedulerStart();//启动调度器

	//正常情况下不会执行到这里
    while (1)
    {

    }
}

```



### 配置

配置选项在schedule.h文件里，

configSysTickClockHz：systick时钟频率，默认是72M

configTickRateHz：中断频率，1000默认是1ms中断一次

alignment_byte：内存管理的字节对齐，默认是八字节对齐

config_heap：任务栈的大小，读者可以根据需要设置

configMaxPriori：任务的最大优先级

configShieldInterPriority：临界区屏蔽的中断最大优先级，由于是高四位有效，因此默认是11

```
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )

#define alignment_byte               0x07
#define config_heap   (8*1024)
#define configMaxPriori 32
#define configShieldInterPriority 191
```



## API的使用（任务管理）

### 任务创建

```
void xTaskCreate( TaskFunction_t pxTaskCode,/*需要执行的任务函数,如果传入形参需要在函数前面加上类型转换，例如														(TaskFunction_t)taskA */
                  uint16_t usStackDepth,//根据任务中的变量、嵌套层数进行配置，真实大小为该值*4个字节
                  void *pvParameters,//形参，默认为NULL
                  uint32_t uxPriority,//优先级,注意的是不能设置同一优先级
                  TaskHandle_t *self );//句柄，需要手动定义
```

写法如下：

```
xTaskCreate(    	taskA,
                    128,
                    NULL,
                    1,
                    &tcbTask1
    );
```



### 任务延时

```
void TaskDelay( uint16_t ticks );
```

在希望延时的任务中添加即可，单位为ms。

### 任务挂起

```
uint32_t StateAdd( TaskHandle_t taskHandle,uint8_t State);
uint32_t StateRemove( TaskHandle_t taskHandle, uint8_t State);
```

这是改变任务状态的API。

**线程的状态**

Sparrow中，任务有五种状态：运行态、就绪态、延时态、阻塞态、挂起态

运行态：任务正在运行，毫无疑问，处在运行态的任务只有一个。当任务处于运行态时，它也处于就绪态。

就绪态：任务可能在运行，也可能准备运行，当任务处于就绪态时，它可能处于运行态（如果它是最高优先级任务的话）。**只有就绪态中的任务会被执行。**

延时态：任务正在延时中，当任务处于延时态时，它还可能处于阻塞态。例如一个任务正在等待一个事件的发生，任务的等待最长时间是100ms，如果事件一直不发生，那么任务就会等完100ms，如果在这个过程中事件发生了，任务会马上执行。在这种情况下，任务既处于延时态，又处于阻塞态。

阻塞态：任务正在等待某个事件的发生，此时任务也可以处于延时态。

挂起态：当任务很长时间都不需要紧急执行时，可以把该任务挂起。当然，不止挂起任务，也可以挂起调度器。挂起态通常是手动设置的，这取决于用户对任务的管理。

**挂起任务**

可以挂起单独的任务，这个任务将不会执行，除非被恢复就绪态。

例如：

```
StateRemove(tcbTask1, Ready);//必须先移除就绪态
StateAdd(tcbTask1, Suspend);
```

当把空闲任务挂起时，调度器会被挂起，可以用来代替临界区。

例如：

```
extern TaskHandle_t leisureTcb;
void taskA(){
	while(1){
		StateAdd(leisureTask, Suspend);//此时调度器会被挂起，不会进行任务切换
	}
}
```



### 临界区

在不希望发生并发问题的代码段前后添加即可，可以嵌套使用。

```
uint32_t  xEnterCritical( void );
void xExitCritical( uint32_t xReturn );
```

例如：

```
uint32_t  xReturn =  xEnterCritical( void );

a = 1;//可能不止一个任务修改a		
		
xExitCritical(xReturn );
```



### 内存管理

```
void *heap_malloc(size_t WantSize);
void heap_free(void *xReturn);
```

这两个函数的使用与C语言的malloc和free一致，不过heap_malloc返回的地址默认八字节对齐的，实际大小也是八字节对齐的，读者可以在配置中修改。

#### 注意

由于内存本身就是一种公共资源，但heap_malloc和heap_free内部并没有并发保护机制，如果在任务运行时动态申请内存，有发生竞态的风险。

如果读者需要在任务运行时动态使用内存，推荐加上临界区保护。

例如：

```
void taskA(void)
{
	while(1){
		uint32_t  xReturn =  xEnterCritical( void );
		void *a = heap_malloc(sizeof(int));
		xExitCritical(xReturn );
		
		a = 1;
		
		uint32_t  xReturn =  xEnterCritical( void );
		heap_free(a);
		xExitCritical(xReturn );
	}
}

```

读者也可以对内存管理的API进行封装：

```
void *os_malloc(size_t WantSize)
{
	uint32_t  xReturn =  xEnterCritical( void );
	void *a = heap_malloc(sizeof(int));
	xExitCritical(xReturn );
}

void os_free(void)
{
	uint32_t  xReturn =  xEnterCritical( void );
	heap_free(a);
	xExitCritical(xReturn );
}
```

这样在任务中就可以直接使用内存管理函数了。

**另一种方法**

竞态来源于并发，读者也可以选择在申请或释放内存时挂起调度器。挂起调度器期间，其他任务不会进行抢占，避免了竞争导致的风险。



## API的使用（任务通信）

### 消息队列

```
Queue_Handle queue_creat(uint32_t queue_length,uint32_t queue_size);//定义队列长度、单个消息的大小
void queue_delete( Queue_Handle queue );//删除队列
uint8_t queue_send(Queue_Handle queue, uint32_t *buf, uint32_t Ticks);//发送消息，Ticks为最大等待时间，单位为ms
uint8_t queue_receive( Queue_Handle queue, uint32_t *buf, uint32_t Ticks );//接收消息，Ticks为最大等待时间，单位为ms
```

分别是创建队列、删除队列、发生消息、接收消息的API。

**基本机制**

发送消息可以一对一、也可以多对多。

发送消息时，未满情况下正常写入，如果消息队列已满，那么会进入等待，如果直到等待时间到期仍然无法写入，那么会退出并返回false。

接收消息时，队列有消息时正常读取，如果没有消息，那么会进入等待，如果直到等待时间到期仍然无法读取，那么会退出并返回false。

推荐写法如下：

```
struct AMessage
{
    char ucMessageID;
    char ucData[ 20 ];
};
struct AMessage xMessage;
struct AMessage aMessage;

//定义接收消息的句柄
Queue_Handle xStructQueue = NULL;

void vCreateQueues( void )
{
    xMessage.ucMessageID = 0xab;
    aMessage.ucMessageID = 0x01;
    memset( &( xMessage.ucData ), 0x12, 20 );
    xStructQueue = queue_creat(1,sizeof( xMessage ) );
}


//Task Area!The user must create task handle manually because of debugging and specification
TaskHandle_t tcbTask1 = NULL;
TaskHandle_t tcbTask2 = NULL;


void taskA( )
{
    while (1) {
        queue_send(xStructQueue,( void * ) &xMessage,1 );
        TaskDelay(1);
    }
}

void taskB( ) 
{
    struct AMessage xRxedStructure;
    while (1) {
        queue_receive( xStructQueue,(void*)(&xRxedStructure),1);
    }
}


void APP( )
{
    xStructQueue = vCreateQueues();//创建消息队列
    
    //创建任务taskA,taskB
}


int main(void)
{
    //bsp初始化
    SchedulerInit();
    APP();
    SchedulerStart();
}
```



### 中断中使用注意

只有任务对象才有延时等待，但中断并不是任务，所以不能设置tick等待时间，在中断中使用时，应当把ticks设置为0。



### 信号量

```
Semaphore_Handle semaphore_creat(uint8_t value);//创建
void semaphore_delete(Semaphore_Handle semaphore);//删除
uint8_t semaphore_release( Semaphore_Handle semaphore);//释放信号量
uint8_t semaphore_take(Semaphore_Handle semaphore,uint32_t Ticks);//获取信号量
```

semaphore_creat(uint8_t value);是创建的函数，value是不同的值，可根据实际情况设置不同的值。

#### 创建时的值不同，功能不同

信号量有两种功能，一种是互斥，另一种是条件同步，决定信号量的功能的关键在于它初始化时的值。

**初始化大于1，计数功能**

假设有五个资源，访问一次少一个，但是有六个任务在同时访问，那么最后那个没有获得资源的任务会进入阻塞等待，最大等待时间就是10ms。

```text

void taskA()
{
	while(1){
		semaphore_take(semaphore, 10);//上锁
		//访问资源的代码
		semaphore_release(semaphore);//解锁
	}
}

void APP()
{
	semaphore = semaphore_creat(5);
	创建任务相关代码
}
```



**初始化为1，互斥功能**

信号量（互斥锁、自旋锁）常被用于互斥，也就是对某个资源的独占访问，此时有多少资源value就设置为多少。

用于互斥时，使用方法如下：

```

void taskA()
{
	while(1){
		semaphore_take(semaphore, 10);//上锁
		//访问资源的代码
		semaphore_release(semaphore);//解锁
	}
}

void APP()
{
	semaphore = semaphore_creat(1);
	//创建任务
}
```



在有些情况下信号量会导致**优先级反转**现象，必须合理分配优先级,也可以使用互斥锁。

其实初始化不为0的情况都可以归类为对资源的独占访问，也就是互斥功能。

**初始化为0，同步功能**

有时候，我们往往会设置一个条件变量，当变量触发时，任务内部的代码才会执行，其实这可以通过信号量实现同步。

使用方法如下：

```text
taskA()
{
    //释放信号量
    semaphore_release(semaphore)
}

taskB()
{   //没获取到信号量时阻塞
    semaphore_take(semaphore, 10)//如果有信号量了，就不阻塞了
    //解除阻塞后，往下执行
}
```

### 互斥锁

```
Mutex_Handle mutex_creat(void);//创建
void mutex_delete(Mutex_Handle mutex);//删除
uint8_t mutex_lock(Mutex_Handle mutex,uint32_t Ticks);//上锁
uint8_t mutex_unlock( Mutex_Handle mutex);//解锁
```



互斥锁就是特殊的互斥型信号量，当程序可能出现优先级反转问题时，可以使用互斥锁。

当超过等待时间时，任务会解除阻塞状态并返回false。

使用方法如下：

```
void code()
{
	mutex_lock(mutex, 10);//上锁
	//访问资源的代码
	mutex_unlock(mutex);//解锁
}

void APP()
{
	mutex = mutex_creat();
}
```

#### 注意

由于Sparrow RTOS并不支持同优先级，因此优先级反转采用的策略是将优先级改为：当前阻塞列表中最大优先级 + 1。

因此使用互斥锁时，设置优先级需要额外注意这一点。



### 生产-消费缓冲队列

生产-消费缓冲队列主要解决以下问题：

单生产者，单消费者

多生产者，单消费者

多生产者，多消费者

总API如下：

```

//单生产者，单消费者
Oo_buffer_handle Oo_buffer_creat(uint8_t buffer_size);
void Oo_insert(Oo_buffer_handle Oo_buffer1, int object);
int Oo_remove(Oo_buffer_handle Oo_buffer1);
void Oo_buffer_delete(Oo_buffer_handle Oo_buffer1);

//多生产者，单消费者
Mo_buffer_handle Mo_buffer_creat(uint8_t buffer_size);
void Mo_insert(Mo_buffer_handle Mo_buffer1, int object);
int Mo_remove(Mo_buffer_handle Mo_buffer1);
void Mo_buffer_delete(Mo_buffer_handle Mo_buffer1);

//多生产者，多消费者
Mm_buffer_handle Mm_buffer_creat(uint8_t buffer_size);
void Mm_insert(Mm_buffer_handle Mm_buffer1, int object);
int Mm_remove(Mm_buffer_handle Mm_buffer1);
void Mm_buffer_delete(Mm_buffer_handle Mm_buffer1);

```



使用如下：

```
void taskA()
{	//针对不同情况使用不同的API
	Mm_insert(MrMw_control1, 1);//把1插入环形队列
}

void taskB()
{
	int b = Mm_remove(MrMw_control1);//读取环形队列
}

void APP()
{
	MrMw_control1 = Mm_buffer_creat(5);//创建一个长度为5的缓存队列
}

int main()
{
	执行代码
}


```

#### 读写锁

读写锁可应用于多读者和多写者的场景，可以解决大部分资源访问问题。

总API如下：

```
rwlock_handle rwlock_creat(void);//创建
void read_acquire(rwlock_handle rwlock_handle1);
void read_release(rwlock_handle rwlock_handle1);
void write_acquire(rwlock_handle rwlock_handle1);
void write_release(rwlock_handle rwlock_handle1);
void rwlock_delete(rwlock_handle rwlock1);//删除
```

使用如下：

```
void taskA() {
    // 针对不同情况使用不同的API
    read_acquire(Oo_buffer1); // 读取
    // 进行读取
    read_release(Oo_buffer1); // 释放，其他任务可以开始读
}

void taskB() {
    write_acquire(Oo_buffer1); // 开始写
    // 进行写操作
    write_release(Oo_buffer1); // 释放，其他任务可以开始写
}

void taskC() {
    read_acquire(Oo_buffer1); // 读取
    // 进行读取
    read_release(Oo_buffer1); // 释放，其他任务可以开始读
}

void taskD() {
    write_acquire(Oo_buffer1); // 开始写
    // 进行写操作
    write_release(Oo_buffer1); // 释放，其他任务可以开始写
}

void APP() {
    Oo_buffer1 = Oo_buffer_creat();
}

int main() {
	//初始化外设
	//初始化调度器
    APP(); //创建任务
	//启动调度器
    
}

```



## 链表版本内核附加功能

链表版本内核是在数表版本内核上的拓展，增加功能如下：

### 链表

提供基本的加入和删除链表节点功能

```
void ListInit(TheList *xList);
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

### 优先级

链表版本内核支持同优先级,且启动第一个任务时会选择优先级最高的任务启动，而数表版本则是选择最后一个创建的任务启动。

### 互斥锁

正常使用互斥锁即可，发生优先级反转时持有锁的任务会暂时提升优先级至阻塞任务中最大优先级。

### 任务

支持任务删除功能：

```
void xTaskDelete(TaskHandle_t self);
```

使用如下：

```
TaskHandle_t tcbTask1 = NULL;
TaskHandle_t tcbTask3 = NULL;

void taskA( )
{
    while (1) {
   
    }
}

void APP( )
{
    xTaskCreate(....);
    xTaskDelete(tcbTask3);//删除启动线程
}

int main(void)
{
	省略初始化代码
    xTaskCreate(    (TaskFunction_t)APP,
                    128,
                    NULL,
                    1,
                    &tcbTask3,
                    0
    );
    SchedulerStart();
}
```

### 原子操作

```
void atomic_add( uint32_t i,uint32_t *v)        //不带return版本，i为操作值，v为被操作数的地址
int atomic_add_return( uint32_t i,uint32_t *v)     

void atomic_sub( uint32_t i,uint32_t *v)        
int atomic_sub_return( uint32_t i,uint32_t *v)  

atomic_inc_return(v)//自增
atomic_dec_return(v) //自减

uint32_t atomic_set_return(uint32_t i, uint32_t *v) //设置被操作数的值
uint32_t atomic_set(uint32_t i, uint32_t *v) 
```

#### 注意

请务必确保被操作数的地址是字节对齐的。



### 定时器

支持定时器，其中定时器的全部回调函数都会由一个线程执行，该线程的优先级、栈大小、多久检查一次都由用户决定。

总API如下：

```
TaskHandle_t xTimerInit(uint8_t timer_priority, uint16_t stack, uint8_t check_period);//初始化创建定时器
TimerHandle xTimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t timer_flag);//创建并添加定时任务
uint8_t TimerRerun(TimerHandle timer);//定时器重新启动
uint8_t TimerStop(TimerHandle timer);//定时器停止
```

参数：

**timer_priority**： 定时器线程的优先级。

**stack**： 定时器线程的栈大小。

**check_period**：定时器检查周期，单位为ms，推荐设置为不同定时任务周期的**最大公约数**。

使用如下：

设置flag参数为0(stop),定时器只会执行一次，设置为1(run)，定时器默认会永远执行，但可以使用TimerRerun和TimerStop决定定时器任务何时启动，何时停止。

```

int a=0;
void count(void)
{
    a++;
}

void APP( )
{
    TaskHandle_t tcbTask3 = xTimerInit(4, 128, 1);//设置定时器的优先级和栈大小，以及检查周期
    xTaskCreate(    (TaskFunction_t)taskA,
                    128,
                    NULL,
                    3,
                    &tcbTask1,
                    0
    );
    xTaskCreate(    (TaskFunction_t)taskB,
                    128,
                    NULL,
                    2,
                    &tcbTask2,
                    0
    );
    TimerHandle timerHandle = xTimerCreat((TimerFunction_t)count,1,stop);
}

```

### 内存池

总API如下：

```
PoolHeadHandle memPool_creat(uint16_t size,uint8_t amount);
void *memPool_apl(PoolHeadHandle ThePool);
void memPool_free(void *xRet);
void memPool_delete(PoolHeadHandle ThePool);
```

推荐写法：

```
void taskA()
{
	while(1){
		int *a = memPool_apl(pool);//从内存池中获得内存块
		*a = 1;
		memPool_free(a);//释放内存块
	}
}


void APP( )
{
	PoolHeadHandle pool = memPool_creat(80,10);//设置内存池大小和个数
    //创建任务等
    TaskCreat(......)

}

```



## 红黑树版本内核

红黑树版本支持同优先级，但不支持时间片功能。

**同优先级支持抢占**

两个任务配置为相同优先级时，后加入就绪树的任务会获得CPU执行权，如果没有同优先级或更高优先级的任务变为就绪态，该任务会一直执行。

**其他功能与链表版本相同。**

调度算法使用缓存指针设计，具有O(1)时间复杂度。

后续会加入动态优先级，使用EDF算法作为实时调度算法。





## 结语

总功能如上，目前的Sparrow RTOS只是一个多任务调度内核，并不具有丰富的服务，不过读者可以根据需要添加或修改某些功能。

如果读者在使用过程中发现了某些bug，或者有某些建议，欢迎反馈！







