# 代码设计

 本文主要讲解SKRTOS_sparrow代码设计风格。

#### Sparrow RTOS开发基本原则

1.跨越多个源文件的全局变量必须带所在文件的名称，局部变量小写加下划线命名，全局变量命名中的不同单词第一个字母应大写

2.不同文件夹相对隔离，仅仅通过必要的接口完成任务

3.尽量让一个模块只负责少量且具有关联性的接口的实现，并且减少与其他模块或类的依赖关系

4.封装内部实现细节，只有公开必要的信息，减少依赖关系

5.不同模块之间尽量避免相互依赖，如果不能，尽量单一依赖。

程序模块化基本设计：短，小，块。

## 面向对象和模块化

面向对象和模块化是SKRTOS_sparrow的主要设计思想。

但是笔者并没有严格采用面向对象思想进行代码的编写，在笔者看来，面向对象只是程序模块化的手段。

以下讲解是笔者在C语言中使用面向对象思想的用法，可用帮助读者理解SKRTOS_sparrow的代码设计，但真实代码并不是严格遵循以下标准。

**以下风格并不是标准的面向对象，只是方便实现模块化设计的手段**

**面向对象是一种思想，不是死板的语法！！！**

## 面向对象

面向对象三大特性：封装、继承、多态。特性实现如下：

### 类

采用宏定义了Class关键字，其实就是简单的对struct进行替换：

```
#define Class(class)    \
typedef struct  class  class;\
struct class
```



### 封装

每个类都会创建一个.c 和.h文件，在.h文件中声明该对象的创建、删除等执行函数。

**公有成员**

公有成员将在.h文件中声明，确保其他文件能访问。

```
.h文件
Class(类)
{
	公有成员：
    PublicMember member1;
    PublicMember member2;
    私有成员：
    PrivateMember member;
};
```

**方法声明**

通过函数指针赋值，通过调用函数指针调用方法：

```
.h
Class(类)
{
公有成员：
    PublicMember member1;
    PublicMember member2;
    
    void (*Init)(   thelist *xlist );
    void (*add)(    thelist *xlist, list_node *newnode );
    void (*remove)( thelist *xlist, list_node *rmnode );

};
```

在.c文件中对该类执行赋值操作：

在不同文件中对函数指针赋值，其实可以实现类似多态的效果。

```
void nodeInit() {
    t_circlet->base.draw = ListnodeInit;  

}
```



**私有成员**

私有成员的封装，是通过将私有成员放在.c文件中，再使用typedef struct完成的，所以在其他文件中，私有成员是不可访问的。

在SKRTOS_sparrow中许多结构体，如信号量，都是被封装在.c文件的，

```
.c文件
Class(PrivateMember)
{
    Member member1;
    Member member2;
};
```

**私有函数**

私有函数将在.c文件中使用static关键字修饰，公有函数则直接在头文件声明。



**私有句柄**

在头文件中，对私有成员的操作可能需要公开，为了避免编译器报错，程序中使用typedef 重命名，如下：

```
.h
typedef struct 私有类 *类_handle;
```

以信号量为例：

信号量的结构体并没有公开，只提供对应的接口。

```
.h
typedef struct Semaphore_struct *Semaphore_Handle;

Semaphore_Handle semaphore_creat(uint8_t value);
void semaphore_delete(Semaphore_Handle semaphore);
uint8_t semaphore_release( Semaphore_Handle semaphore);
uint8_t semaphore_take(Semaphore_Handle semaphore,uint32_t Ticks);
```



### 继承

继承其他类，只需要在类中声明对应结构体即可：

```

Class(类)
{
    struct 类;//继承
    Member member2;
};
```

### 多态

SKRTOS_sparrow中有些地方体现了多态思想，当然，这并不是严格的多态，因为缺乏类和继承的完整操作。

但正如笔者所说：**面向对象只是思想，不是死板的语法。**

使用函数指针完成类似多态的操作，通常的做法是建立一个函数指针数组，并使用数组下标判断执行具体的操作，类似于虚函数表。

以list.c中的链表插入函数为例：

通过传入的rt的值，判断需要对新加入的节点执行哪一种操作。

```

void dataflow(uint8_t rt,TheList *xList, ListNode *new_node)
{
    void (*ListInsert[])(TheList *xList, ListNode *new_node) = {
            InsertFirst,
            InsertHead,
            InsertTail,
            InsertMiddle,
    };
    ListInsert[rt](xList, new_node);

}
```



## 模块化设计

SKRTOS_sparrow通过面向对象思想，把内核中的各个结构体用.c与.h文件进行封装，从而完成模块化设计。

但是，**面向对象只是思想，不是死板的语法。**

程序讲究的是高内聚，低耦合，在某些程序相关性特别强的代码处，需要把程序写成块。

例如在schedule.c文件中，调度器的设计是特别复杂的，包含时钟、上下文切换、任务创建、调度算法等代码。强行把这些代码拆开，划分为类，不但会大大影响性能，还会使程序复杂性增加。

对于耦合性非常低的模块，采用严格的模块化设计，尽量避免依赖，此时可以使用面向对象思想。但是对于重要的程序区域，需要仔细思考程序的设计。

程序的性能与可维护性是第一重要的，但在内核设计中，笔者偏向性能这一端。

综上，SKRTOS_sparrow的代码设计思想就讲到这里，后面会继续补充。





