# SKRTOS_sparrow

SKRTOS_sparrow内核的定位是学习型RTOS，项目的重点在于内核设计相关的文档教程。

### 2025.3.16留言

本项目暂停维护更新。

## 说明

项目下有两个分支：

**main**:内核源码、支持的架构、已经移植的工程。

**study**：在该分支下有《400行程序写一个RTOS》pdf与对应的实验工程。如果读者对学习如何写一个RTOS感兴趣，请移步到另一个分支study：[skaiui2/SKRTOS_sparrow at study](https://github.com/skaiui2/SKRTOS_sparrow/tree/study)



### 当前分支的目录

arch:不同的架构移植接口

boards：不同开发板的移植工程

docs：文档，包括内核设计与源码讲解

kernel：源码



## 版本

内核现在具有四个版本：数表版本(Table)、链表版本(List)、红黑树版本(RBtree)、响应EDF版本（rbtreeEDF）。

也可以把这四个版本划分为：学习版、使用版、实验版、尝试版。

四个版本中支持的编译器和平台正在维护更新中，目前默认支持的架构是arm cm3，默认支持的编译器为gcc。

#### 数表版本

只支持32个以下的任务，且不支持同优先级，该版本支持keil、gcc、IAR多种编译器，其中不带IPC版本仅400行代码。

[使用手册](UserManual/tableUser.md)

#### 链表版本

支持同优先级，对支持的任务数量没有限制，该版本接口部分可参考数表版本进行修改。

#### 红黑树版本

支持同优先级，但不支持时间片，其他功能与链表版本相同。

以上三个版本的设计思想并没有任何区别。

#### 响应EDF版本

在传统EDF算法上的改进，强调系统的周期性与可预测性，支持arm cortex A7架构。

另外三个版本：[使用手册](UserManual/other.md)

**适用范围**

数表版本：学习用途或资源有限的条件。

链表版本：完成度较高，适合大多数场景。

红黑树版本：适用于大量任务频繁搜索、插入和删除操作的场景。

响应EDF版本：适用于强调周期性与可预测性的场景，目前只进行了一些简单的测试，功能还不稳定，目前只是一个玩具。

### 版本说明

**本项目不再进行较大更新，而是注重于维护目前的四个版本**，接下来要做的工作有：

1.**优化不同版本的代码**，完善功能，修复bug。

2.**编写详细的文档**，包括使用文档、学习文档等一系列文档。

### 各版本优化方向

**数表版本**：适用场景是低资源与高效率，内核应当精简，后期会使用大量的位操作加速运算，并进行冗余代码优化，对变量进行复用提高效率。

**链表版本**：适用于一般场景，内核追求稳定性，应当向流行的RTOS看齐，如FreeRTOS。

**红黑树版本**：适用于大量线程的高并发场景，强调”万物皆可锁“的概念，后期会加入大量的锁保证并发的安全性，对锁与并发的管理应当学习Linux内核。

**响应EDF版本**：适用于强调周期性、可预测性的场景，在参考红黑树版本锁管理的基础上，应当添加任务可调度性的计算程序与预测程序，后期会改进调度算法，预测程序会参考TCP/IP的RTT估计器等算法进行修改。

目前各个版本大同小异，但是优化方向是不一样的，考虑到后期的变动，对不同版本的文档也进行了划分。



## 文档说明

本分支下的**docs文件夹**包含了SKRTOS_sparrow内核的设计说明，

**ArithmeticOptimizations文件夹**：一些运算和程序优化相关的理解。

**CodeDesign文件夹**：代码的设计风格、编程思想。

**Kernel_imple文件夹**：内核的具体实现，文件中的具体代码的讲解，每周末会更新一篇文章。

包括以下内容：

1.内核各个文件的代码是什么？内核的代码风格是什么？

2.内核是如何设计的？如何从0到1写一个RTOS内核？

3.这一行行代码是在干什么？

4.CPU架构、实时操作系统相关知识。

5.对特定问题的理解与思考



### 文档更新

Kernel_imple文件夹已更新文章：

1.链表版本、红黑树版本、响应EDF版本的定时器实现。

2.各个版本的内存管理实现。



