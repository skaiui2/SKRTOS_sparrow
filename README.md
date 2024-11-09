# SKRTOS_sparrow


### 移植

这里是Sparrow的源码，对于arm cm3架构的单片机(例如stm32f103c8t6)，只需要将其复制到原工程的文件中，并且删除掉stm32f1xx_it.c文件中的SVC_Handler、SysTick_Handler和PendSV_Handler中断，即可成功完成移植。



Sparrow.c适用于clion的gcc编译器和keil v6版本的编译器。

KeilSparrow.c使用于keil v5版本的编译器。

SparrowIPC.c是Sparrow.c的拓展，引入了IPC机制，使Sparrow成为一个较为完整的内核。

Sparrow文件夹里的是对SparrowIPC.c源码进行分层封装并抽象后的源码版本。

### 移植注意

在keil环境下，当移植完Sparrow RTOS后，如果编译没报错，但是程序不能运行，请尝试降低keil的编译器优化等级。这是因为hal生成的工程默认编译优化全开，可能导致出现bug。



### 教程

关于Sparrow的文档教程已经完结了。

知乎链接：[400行程序写一个实时操作系统（开篇） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/963319443)
