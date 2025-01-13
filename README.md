# SKRTOS_sparrow


### 移植

这里是Sparrow的源码，对于arm cm3架构的单片机(例如stm32f103c8t6)，只需要将其复制到原工程的文件中，并且删除掉stm32f1xx_it.c文件中的SVC_Handler、SysTick_Handler和PendSV_Handler中断，即可成功完成移植。



**Sparrow.c**适用于clion的gcc编译器和keil v6版本的编译器。

**KeilSparrow.c**适用于keil v5版本的编译器。

**SparrowIPC.c**是Sparrow.c的拓展，引入了IPC机制信号量，使Sparrow成为一个较为完整的内核。

**Sparrow**文件夹里的是对Sparrow源码进行**分层封装并抽象**后的源码版本，具有**消息队列、信号量、互斥锁**三个IPC机制。

**SparrowIAR**文件夹里的是Sparrow.c的IAR版本，将接口代码放在了port.s文件里。

**SparrowList**文件夹里是链表版本的Sparrow RTOS，添加了分时时间片、原子操作等功能，任务较多时（超过32个），推荐移植该版本，该版本功能尚不稳定，欢迎读者反馈使用情况。

### 使用手册

[使用手册](USER_MANUAL.md)

## 注意！！！

**由于维护更新可能不及时，部分版本可能存在bug，如果遇到移植问题，欢迎反馈！**

**推荐移植：**  **Sparrow文件夹**

**目前链表版本的Sparrow RTOS尚未在使用手册中更新API说明，过段时间会对项目分支重新进行整理，并将教程整理上传，与教程的实验工程单独作为一个分支。**



### 移植注意

在keil环境下，当移植完Sparrow RTOS后，如果编译没报错，但是程序不能运行，请尝试降低keil的编译器优化等级。这是因为hal生成的工程默认编译优化全开，可能导致出现bug。



### 教程

关于Sparrow的文档教程已经完结了。

知乎链接：[400行程序写一个实时操作系统（开篇） - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/963319443)
