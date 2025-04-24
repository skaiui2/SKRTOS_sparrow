# 数表版本使用手册

## 移植

找到main分支下的kernel\table文件夹，再根据自己的开发板架构找到arch\arm文件夹，根据自己的需要选择合适的接口文件。

将这些文件添加到原工程的文件中。

对于arm cm3架构：以stm32f1为例，删除掉stm32f1xx_it.c文件中的SVC_Handler、SysTick_Handler和PendSV_Handler中断，即可成功完成移植。



## 支持的功能

内存：支持小内存管理与内存池。

任务：支持就绪、挂起、延时、删除等功能。

调度器：优先级抢占式运行，支持挂起调度器。

IPC机制：消息队列、信号量、互斥锁、缓存队列、读写锁。

额外功能：定时器、原子操作、临界区。





### 配置

配置选项在schedule.h与port文件夹里的config文件，

**schedule.h**

configMaxPriority：任务的最大优先级及支持任务的个数。

alignment_byte：内存管理的字节对齐，默认是八字节对齐，如果要修改为其他对齐，设置为：对齐数 - 1即可。

config_heap：任务栈的大小，读者可以根据需要设置。

configTimerNumber：定时器个数。

```
//config
#define alignment_byte               0x07
#define config_heap   (10240)
#define configMaxPriority 32
#define configTimerNumber  32
```

**config.h**

configSysTickClockHz：systick时钟频率，默认是72M。

configTickRateHz：中断频率，1000默认是1ms中断一次。

configShieldInterPriority：临界区屏蔽的中断最大优先级，由于是高四位有效，因此默认是11。

```
#define configSysTickClockHz			( ( unsigned long ) 72000000 )
#define configTickRateHz			( ( uint32_t ) 1000 )
#define configShieldInterPriority 191
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
    TaskCreate(     taskA,//需要执行的任务函数
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







## API的使用（任务管理）

### 任务的状态

内核中，任务有六种状态：运行态、就绪态、延时态、阻塞态、挂起态、死亡态

运行态：任务正在运行，毫无疑问，处在运行态的任务只有一个。当任务处于运行态时，它也处于就绪态。

就绪态：任务可能在运行，也可能准备运行，当任务处于就绪态时，它可能处于运行态（如果它是最高优先级任务的话）。**只有就绪态中的任务会被执行。**

延时态：任务正在延时中，当任务处于延时态时，它还可能处于阻塞态。例如一个任务正在等待一个事件的发生，任务的等待最长时间是100ms，如果事件一直不发生，那么任务就会等完100ms，如果在这个过程中事件发生了，任务会马上执行。在这种情况下，任务既处于延时态，又处于阻塞态。

阻塞态：任务正在等待某个事件的发生，此时任务也可以处于延时态。

挂起态：当任务很长时间都不需要紧急执行时，可以把该任务挂起。当然，不止挂起任务，也可以挂起调度器。挂起态通常是手动设置的，这取决于用户对任务的管理。

死亡态：任务被标记为死亡，将会由空闲任务回收。



### 任务创建

```
void  TaskCreate( TaskFunction_t pxTaskCode,/*需要执行的任务函数,如果传入形参需要在函数前面加上类型转换				
                  uint16_t usStackDepth,//根据任务中的变量、嵌套层数进行配置，真实大小为该值*4个字节
                  void *pvParameters,//形参，默认为NULL
                  uint32_t uxPriority,//优先级,注意的是不能设置同一优先级
                  TaskHandle_t *self );//句柄，需要手动定义
```

写法如下：

```
 TaskCreate(    	taskA,
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
void TaskSusPend(TaskHandle_t self);
void TaskResume(TaskHandle_t self);
```

可以挂起单独的任务，这个任务将不会执行，除非被恢复就绪态。

例如：

```
extern TaskHandle_t leisureTcb;
void taskA(){
	while(1){
		TaskSusPend(leisureTask);
	}
}
```

#### 调度器挂起

```
void SchedulerSusPend(void);
```

调度器会被挂起，不会进行任务切换

### 任务删除

```
void TaskDelete(TaskHandle_t self);
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
    TaskCreate(....);
    TaskDelete(tcbTask3);//删除启动线程
}

int main(void)
{
	省略初始化代码
     TaskCreate(    (TaskFunction_t)APP,
                    128,
                    NULL,
                    1,
                    &tcbTask3,
                    0
    );
    SchedulerStart();
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

这两个函数的使用与C语言的malloc和free一致，不过heap_malloc返回的地址默认八字节对齐的，实际分配的内存块大小也是八字节对齐的，读者可以在配置中修改。

#### 注意

由于内存本身就是一种公共资源，但heap_malloc和heap_free内部并没有并发保护机制。因此在任务运行时动态申请内存，可能会发生竞态，这将导致程序出现错误，所以如果读者需要在任务运行时动态使用内存，请加上临界区保护。

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

### 



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



在有些情况下信号量会导致**优先级反转**现象，必须合理分配优先级。当然，这种情况也可以使用互斥锁。

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

由于内核并不支持同优先级，因此优先级反转采用的策略是：如果该互斥锁的阻塞任务优先级比互斥锁持有者高，那么持有互斥锁的任务会立刻抢占CPU执行任务并释放锁。



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
    rwlock_handle rwlock = rwlock_creat();
}

int main() {
	//初始化外设
	//初始化调度器
    APP(); //创建任务
	//启动调度器
    
}

```





### 定时器

支持定时器，其中定时器的全部回调函数都会由一个线程执行，该线程的优先级、栈大小、多久检查一次都由用户决定。

总API如下：

```
TaskHandle_t TimerInit(uint8_t timer_priority, uint16_t stack, uint8_t check_period);
TimerHandle TimerCreat(TimerFunction_t CallBackFun, uint32_t period, uint8_t index, uint8_t timer_flag);
uint8_t TimerRerun(TimerHandle timer, uint8_t timer_flag);
uint8_t TimerStop(TimerHandle timer);
uint8_t TimerStopImmediate(TimerHandle timer);
void TimerDelete(TimerHandle timer);
```



**TimerInit函数**

初始化定时器，创建定时器线程。

timer_priority： 定时器线程的优先级。

stack： 定时器线程的栈大小。

check_period：定时器检查周期，单位为ms，推荐设置为不同定时任务周期的**最大公约数**。

**TimerCreat函数**

创建定时器并加入定时器到数组中。

CallBackFun：需要执行的回调函数。

period：周期。

index：添加到定时器数组后的下标，其中数组最大数可在schedule.h文件配置。

timer_flag：设置flag参数为0(stop),定时器只会执行一次，设置为1(run)，定时器默认会永远执行。

**TimerRerun函数**

将定时器重新加入列表，重新运行，可设置标志选择继续运行一次或永久。

**TimerStop函数**

定时器再执行一次后就移除定时器列表。

**TimerStopImmediate函数**

将定时器立刻移除列表。

**TimerDelete函数**

删除定时器。

使用如下：

```

int a=0;
void count(void)
{
    a++;
}

void APP( )
{
    TaskHandle_t tcbTask3 = TimerInit(4, 128, 1);//设置定时器的优先级和栈大小，以及检查周期
    TaskCreate(     (TaskFunction_t)taskA,
                    128,
                    NULL,
                    3,
                    &tcbTask1,
                    0
    );
    TaskCreate(    (TaskFunction_t)taskB,
                    128,
                    NULL,
                    2,
                    &tcbTask2,
                    0
    );
    TimerHandle timerHandle = TimerCreat((TimerFunction_t)count, 1, 0, stop);
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





## 结语

总功能如上，目前的Sparrow RTOS只是一个多任务调度内核，并不具有丰富的服务，不过读者可以根据需要添加或修改某些功能。

如果读者在使用过程中发现了某些bug，或者有某些建议，欢迎反馈！







