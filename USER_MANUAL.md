# Sparrow RTOS使用手册

## 简介

Sparrow RTOS是一个极简型RTOS内核，定位是学习型RTOS。

项目地址：[skaiui2/SKRTOS_sparrow: Lightweight rtos inspired by SKRTOS](https://github.com/skaiui2/SKRTOS_sparrow)

在项目下有五个分支:

main：不同平台的已移植工程，例如：keil、IAR、clion

experiment：实验工程，需要搭配教程。

manual：arm cm3和arm cm4的手册。

memory：内存管理相关的实验工程及单独的内存管理函数库。

source：Sparrow RTOS的源码，主要维护和更新该分支。



教程地址：[400行程序写一个实时操作系统（开篇） - 知乎](https://zhuanlan.zhihu.com/p/963319443)

教程不仅讲解原理，也指导读者如何一步步完成一个RTOS内核。



## 移植

将其添加到原工程的文件中，并且删除掉stm32f1xx_it.c文件中的SVC_Handler、SysTick_Handler和PendSV_Handler中断，即可成功完成移植。

**推荐移植：source分支下的Sparrow文件夹版本。**

### 不同平台的移植问题

Sparraw RTOS目前只适用于arm cm3架构，考虑到不同平台的移植问题，在设计时，笔者将Sparrow RTOS的接口定义与FreeRTOS内核接口一致，如果要移植到不同架构和平台，可以直接参考FreeRTOS的实现。

移植接口主要是内联汇编部分，由于不同平台对内联汇编的支持不一样，这部分汇编代码需要读者结合具体的开发环境进行修改。

#### 移植时架构接口：

vPortSVCHandler、xPortPendSVHandler、pxPortInitialiseStack。这三个接口与具体架构有关，可以参考FreeRTOS相关架构的实现进行修改。

#### 临界区接口：

xEnterCritical、 xExitCritical临界区函数也与具体架构的中断屏蔽寄存器有关，也可以参考一些开源的RTOS代码进行修改。

#### 汇编接口：

FindHighestPriority函数使用clz指令寻找最高优先级任务，可以修改C语言版本，也可以结合具体编译器进行修改，例如：

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
xTaskCreate(    taskA,
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

由于内存本身就是一种公共资源，heap_malloc和heap_free内部并没有并发保护机制。因此如果在任务运行时需要动态内存空间时，有发生并发导致竞态的风险，请加上临界区保护。

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
	//创建任务
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



### 信号量demo

信号量demo是信号量的应用，主要解决以下问题：

单生产者，单消费者

多生产者，单消费者

多生产者，多消费者

多读者，多写者

**总API如下：**

推荐使用多读者，多写者的API，可以解决大部分并发访问的问题，包括生产者和消费者问题。

```
//单生产者，单消费者
Oo_buffer_handle Oo_buffer_creat(void);
void Oo_insert(Oo_buffer_handle Oo_buffer1, int object);
int Oo_remove(Oo_buffer_handle Oo_buffer1);

//多生产者，单消费者
Mo_buffer_handle Mo_buffer_creat(void);
void Mo_insert(Mo_buffer_handle Mo_buffer1, int object);
int Mo_remove(Mo_buffer_handle Mo_buffer1);

//多生产者，多消费者
Mm_buffer_handle Mm_buffer_creat(void);
void Mm_insert(Mm_buffer_handle Mm_buffer1, int object);
int Mm_remove(Mm_buffer_handle Mm_buffer1);

//多读者，多写者
MrMw_control_handle MrOw_creat(void);
void read_acquire(MrMw_control_handle MrMw_control1);
void read_release(MrMw_control_handle MrMw_control1);
void write_acquire(MrMw_control_handle MrMw_control1);
void write_release(MrMw_control_handle MrMw_control1);

```



以消费者、生产者模型为例：

Sparrow RTOS的消费者、生产者的实际模型是一个环形缓冲区，在一些场景下可以代替消息队列。

```
#define BufferSIZE 5
Class(Oo_buffer)
{
    int buf[BufferSIZE];
    int in;
    int out;
    Semaphore_Handle item;
    Semaphore_Handle space;
};
```

使用者可以通过修改BufferSIZE的大小，配置环形缓冲区的大小。

使用如下：

```
void taskA()
{	//针对不同情况使用不同的API
	write_acquire(MrMw_control1, 1);//把1插入环形队列
}

void taskB()
{
	int b = read_acquire(MrMw_control1);//读取环形队列
}

void APP()
{
	MrMw_control1 = MrOw_creat();
}

int main()
{
	
}


```

#### 多读者，多写者

多读者，多写者的API可以解决大部分资源访问问题。

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



## 结语

总API功能如上，Sparrow RTOS只是一个多任务调度内核，并不具有丰富的服务，不过读者可以根据需要添加或修改某些功能。

如果读者在使用过程中发现了某些bug，或者有某些建议，欢迎反馈！