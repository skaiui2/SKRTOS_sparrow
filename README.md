# SKRTOS_sparrow

## Introduction

SKRTOS_sparrow is a Real-Time Operation System micro kernel.  

#### NOTES

All of my remaining time is dedicated to working on the TCP/IP stack and don’t have much time to fix the kernel's bugs. I might need to wait until I have more time to address these bugs.

**Contact information**

skaiuijing@gmail.com

### Description

#### Directory of the current branch

**arch**: Interface for porting different architectures

**boards**: Transplant project of different development boards

**docs**: Documentation, including kernel design and source code

**kernel**: source code

**lib**: data structure and algorithm library



#### Tutorial

If you want to write a micro kernel, there are two kinds of tutorial in **Chinese** and **English**.

**study**：

**中文**：[skaiui2/SKRTOS_sparrow at study](https://github.com/skaiui2/SKRTOS_sparrow/tree/study)

**English** : 

Part1: Memory management

[Let’s write a Real-Time Operating System(RTOS) (Part 1: Memory management) | by Skaiuijing | Apr, 2025 | Medium](https://medium.com/@skaiuijing/lets-write-a-real-time-operating-system-rtos-part-1-5873f6c2184f)

Part2: Context switch and Multithread

[Let’s write a Real-Time Operating System(RTOS) (Part 2: Context switch and Multithread) | by Skaiuijing | Apr, 2025 | Medium](https://medium.com/@skaiuijing/lets-write-a-real-time-operating-system-rtos-part-2-8cc3cd11c8cf)

Part3: Multi-priority preemptive Scheduler

[Let’s write a Real-Time Operating System(RTOS) (Part3: Multi-priority preemptive Scheduler) | by Skaiuijing | Apr, 2025 | Medium](https://medium.com/@skaiuijing/lets-write-a-real-time-operating-system-rtos-part3-multi-priority-preemptive-scheduler-167152ce9719)

Part4: Concurrency and interrupt management

[Let’s write a Real-Time Operating System(RTOS) (Part4: Concurrency and interrupt management) | by Skaiuijing | May, 2025 | Medium](https://medium.com/@skaiuijing/lets-write-a-real-time-operating-system-rtos-part4-concurrency-and-interrupt-management-2ef184ac1e42)

Part5: Time triggered scheduling

[Let’s write a Real-Time Operating System(RTOS) (Part 5: Time triggered scheduling) | by Skaiuijing | May, 2025 | Medium](https://medium.com/@skaiuijing/lets-write-a-real-time-operating-system-rtos-part-5-clock-triggered-scheduling-fcd433285183)

Part6: Timer

[Let’s write a Real-Time Operating System(RTOS) (Part6: Timer) | by Skaiuijing | May, 2025 | Medium](https://medium.com/@skaiuijing/lets-write-a-real-time-operating-system-rtos-part6-timer-b7e9be931ec3)

Part7: Semaphore and Mutex

Part8: Message queue

Part9: Read-Write lock

Part10: Conclusion



## version

The kernel now has four versions: the  **Table version** (Table), the **List version** (List), the **red-black tree version** (RBtree), and the r**esponse EDF version** (rbtreeEDF).

These four versions can also be divided into: learning version, use version, experimental version, and trial version.

The compilers and platforms supported in the four releases are under maintenance update.

### Table version

Only support less than 32 tasks, and do not support the same priority.

**USER MANUAL**

[中文](UserManual/中文/tableUser.md)

[English](UserManual/English/tableUser.md)

#### Linked list version

There is no limit on the number of supported tasks. The interface of this version can be modified by referring to the number table version.

#### Red and black tree version

Supports the same priority, but does not support time slices, and other features are the same as the linked list version.

The above three versions of the design idea is not any difference.

#### Responds to the EDF version

The improvement on the traditional EDF algorithm emphasizes the periodicity and predictability of the system, and supports the arm cortex A7 architecture.

**USER MANUAL**

Three other versions: [中文](UserManual/English/other.md)

[English](UserManual/English/other.md)

### Scope of Application

table version: Learning conditions with limited use or resources.

list version: high degree of completion, suitable for most scenarios.

Red-black tree version: applicable to scenarios where a large number of tasks are frequently searched, inserted, and deleted.

Response EDF version: Suitable for scenarios that emphasize periodicity and predictability, only some simple tests have been conducted, the function is not stable, and it is only a toy at present.



### Each version optimization direction

**table version** : The application scenario is low resources and high efficiency, the kernel should be streamlined, and a large number of bit operations will be used in the later stage to accelerate the operation, and redundant code optimization, and the reuse of variables to improve efficiency.

**list version** : Suitable for general scenarios.

**Red-black tree version** : suitable for high concurrency scenarios with a large number of threads, emphasizing the concept of "everything can be locked", and a large number of locks will be added later to ensure the security of concurrency.

**Response EDF version** : Suitable for scenarios that emphasize periodicity and predictability. On the basis of referring to red-black tree version lock management, a calculation program and prediction program for task schedulability should be added. The scheduling algorithm will be improved later, like add the prediction program.



## Documentation

The docs folder  under this branch contains the SKRTOS_sparrow kernel design instructions,

ArithmeticOptimizations folder  : Some understanding of arithmetic and program optimization.

CodeDesign folder  : Code design style, programming ideas.

Kernel_imple folder  : The specific implementation of the kernel, the specific code in the file.

