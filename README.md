# SKRTOS_sparrow
Lightweight rtos inspired by SKRTOS



### 说明

这里是sparrow已经移植好了的工程，适用于arm cm3架构（例如stm32f103c8t6）。

**开发环境是clion：**

**SparrowHAL**是hal库的移植，**SparrowStdlib**是标准库的移植。



**开发环境是Keil：**

**SparrowKeilStd**是在keil环境下能稳定运行的标准库工程，适用于keil v5.06版本的编译器。**SparrowKeilHal**是在keil环境下能稳定运行的HAL库工程,	适用于Keil V6版本的编译器。

如果编译没问题但是运行错误，请尝试降低keil的编译器优化等级。这是因为hal生成的工程默认编译优化全开，可能导致出现bug。



**开发环境是IAR：**

**SparrowIAR**是在IAR环境下能稳定运行的hal库版本。



**以上文件夹的Sparrow RTOS都是400行源代码的版本**

### 其余三个文件夹

**SparrowMeque**是**消息队列**的测试工程

**SparrowSem**是**信号量**的测试工程

**SparrowMutex**是**互斥量**的测试工程

三个文件夹均带有Sparrow的**消息队列、信号量、互斥锁**三个IPC机制，是Sparrow的**拓展版本**，IPC机制的源码都具有良好的可移植性。

带有IPC机制的工程中的Sparrow文件夹是Sparrow分层封装并进行抽象后的工程，将调度层与IPC层隔离开来，调度层实现任务相关的接口，IPC层仅仅利用线程状态转移相关的接口实现线程通信机制。

Sparrow的源码开发遵循以下原则，其中接口隔离原则是核心：

#### Sparrow开发基本原则

1.跨越多个源文件的全局变量必须带所在文件的名称，局部变量小写加下划线命名，全局变量命名中的不同单词第一个字母应大写

2.不同文件夹相对隔离，仅仅通过必要的接口完成任务

3.尽量让一个模块只负责少量且具有关联性的接口的实现，并且减少与其他模块或类的依赖关系

4.封装内部实现细节，只有公开必要的信息，减少依赖关系

5.不同模块之间尽量避免相互依赖，如果不能，尽量单一依赖。



### 教程

关于Sparrow的文档教程已经完结了。

知乎链接：[400行程序写一个实时操作系统（开篇） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/963319443)
