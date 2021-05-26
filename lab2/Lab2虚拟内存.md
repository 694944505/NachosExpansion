# 虚拟内存实习报告



[TOC]

## 内容一：总体概述

本次实验通过阅读并修改**Nachos**底层源码，达到“实现虚拟存储系统”的目标。第一部分的主要内容是实现TLB的异常处理及相应的置换算法，第二部分实现全局内存管理机制和多线程，第三部分实现**Lazy-loading**。

## 内容二：任务完成情况

### 任务完成列表（Y/N）

| Exercise1     | Exercise2     | Exercise3     | Exercise4      |                |
| ------------- | ------------- | ------------- | -------------- | -------------- |
| Y             | Y             | Y             | Y              |                |
| **Exercise5** | **Exercise6** | **Exercise7** | **Chanllege1** | **Chanllege2** |
| Y             | Y             | Y             | N              | Y              |

​                             

### 具体Exercise的完成情况

#### 第一部分、TLB异常处理

#####Exercise 1 源代码阅读

-   userprog/progtest.cc

> Nachos 系统执行用户程序的文件，主要定义了两个函数
>
> 1. void StartProcess(char *filename):
>
>    打开程序文件，通过AddrSpace类分配地址空间并初始化，然后调用machine类的Run函数运行用户程序
>
> 2. void ConsoleTest (char *in, char *out)
>
>    控制台输入输出的测试函数

-   machine/machine.h和machine/machine.cc

> 定义并实现了用于模拟nachos用户程序的Machine类，类中定义了存储结构和寄存器。还定义了指令类Instruction，引用了一个外部函数ExceptionHandler进行异常处理，实现了一些用于字节数据转换的函数。

-   machine/ translate.h和machine/ translate.cc

> 定义了页表项类TranslationEntry，用于实现线性页表和TLB，每个表项记录地址映射关系和状态信息。实现了Machine类的以下成员函数
>
> 1. bool Machine::ReadMem(int addr, int size, int *value):
>
>    从地址addr读入size字节的数据到value
>
> 2. bool Machine::WriteMem(int addr, int size, int value):
>
>    把value的size字节数据写入地址addr
>
> 3. ExceptionType Machine::Translate(int virtAddr, int* physAddr, int size, bool writing):
>
>    将虚拟地址映射到物理地址，返回异常

-   userprog/ exception.cc

> 实现了void ExceptionHandler(ExceptionType which)函数：用来处理各类异常。



#####Exercise2 TLB MISS异常处理

**修改code/userprog目录下exception.cc中的ExceptionHandler函数，使得Nachos系统可以对TLB异常进行处理（TLB异常时，Nachos系统会抛出PageFaultException，详见code/machine/machine.cc）**

修改ExceptionHandler函数，判断exception为PageFaultException时，调用Machine类的SwapTLB函数进行TLB的置换。更改machine/ translate.cc下的ReadMem和WriteMem函数，当Translate返回PageFaultException的时候，处理完异常后重新调用一次Translate。结果和**Exercise3**一起展示。

#####Exercise3 置换算法

**为TLB机制实现至少两种置 换算法，通过比较不同算法的置换次数可比较算法的优劣。**

本次lab实现了FIFO和LRU两种调度算法。

TLB也是用TranslationEntry类实现的，所以在TranslationEntry类里添加两个成员firstUseTime和lastUseTime，分别记录TLB第一次装入新内容的时刻和其最后一次使用的时刻。在Machine类中添加用于统计的成员变量tlbHit和tlbMiss，在程序退出时打印统计信息。

更改test/sort.c函数，将数组长度设置为20，重新编译后测试如下。

![1](C:\Users\69494\Desktop\软微\操统\lab2\images\1.PNG)

其中./test/sort后的参数0、1分别表示FIFO、LRU策略，可以看出LRU策略的效果比较好。

#### 第二部分、分页式内存管理

#####Exercise4 内存全局管理数据结构

**设计并实现一个全局性的数据结构（如空闲链表、位图等）来进行内存的分配和回收，并记录当前内存的使用状态。**

由于nachos里已有一个BitMap数据结构，因此直接使用。在Machine类中添加一个bitmap的对象，大小为NumPhysPages；添加allocMemory和freeMemory成员函数进行物理内存的分配和回收，在AddrSpace类中申请内存，在userprog/ exception.cc的ExceptionHandler函数中处理SC_Exit异常时回收内存。

#####Exercise5 多线程支持

**目前Nachos系统的内存中同时只能存在一个线程，我们希望打破这种限制，使得Nachos系统支持多个线程同时存在于内存中。**

有了内存全局管理结构之后，内存中单线程的限制就打破了。测试时，修改halt程序为直接调用Exit(0)，在userprog/progtest.cc的StartProcess中使用两个线程分别运行halt程序，结果如下。（参数-mt参数表示这是一次多线程测试）

![2](C:\Users\69494\Desktop\软微\操统\lab2\images\2.PNG)

#####Exercise6 缺页中断处理

**基于TLB机制的异常处理和页面替换算法的实践，实现缺页中断处理（注意！TLB机制的异常处理是将内存中已有的页面调入TLB，而此处的缺页中断处理则是从磁盘中调入新的页面到内存）、页面替换算法等。**

在之前的部分中，进程所需要的代码和数据全部装入到了内存且建立了页表，所以此处的缺页中断需要结合下面的Lazy-loading来做

#### 第三部分、Lazy-loading

#####Exercise7

**我们已经知道，Nachos系统为用户程序分配内存必须在用户程序载入内存时一次性完成，故此，系统能够运行的用户程序的大小被严格限制在4KB以下。请实现Lazy-loading的内存分配算法，使得当且仅当程序运行过程中缺页中断发生时，才会将所需的页面从磁盘调入内存。**

首先通过FileSystem创建一个文件来模拟磁盘，修改Addspace类的构造函数，将用户程序的内容先装载到模拟的磁盘，等缺页中断发生时再装入内存。在Addspace类加入成员函数getPTE，负责将所需的页面从磁盘调入内存，并更新页表。当内存占满时，根据LRU策略把旧的页表项对应的物理内存写入磁盘。使用nachos源码自带的/test/matmult程序进行测试结果如下

![3](C:\Users\69494\Desktop\软微\操统\lab2\images\3.PNG)

![4](C:\Users\69494\Desktop\软微\操统\lab2\images\4.PNG)

#### 第四部分、Challenges

##### Challenge 1

未完成

#####Challenge 2

倒排页表是一个全局性的页表，因此要在Machine类中创建和维护。在TranslationEntry类中增加threadID变量记录线程ID，修改userlog/addrspace.cc下的RestoreState函数（在倒排页表中进程切换时不需要更换页表）和getPTE函数。倒排页表在查找时要遍历整个页表，找逻辑地址、线程ID都符合查找条件的表项，按这个思路修改machine/translate.cc下的读取页表项的部分，修改machine/machine.cc下的SwapTLB函数，使用**Exercise5**中的多线程任务测试如下

![5](C:\Users\69494\Desktop\软微\操统\lab2\images\5.PNG)

## 内容三：遇到的困难以及解决方法

1. 设计虚拟内存时没有模拟出统一的磁盘

   > 对每个进程空间利用FileSystem分别模拟了一个磁盘空间。

2. 代码过多，Debug时无法快速定位到出错点。

   > 多用ASSET进行一些必要的判断。

3. 完成虚拟内存之后再测试**Exercise5**，发现输出变了

   ![5](C:\Users\69494\Desktop\软微\操统\lab2\images\6.PNG)

   > 这是因为第一个线程用了虚拟内存之后实际上只用了三个页，而第二个线程需要的数据和第一个是一样的，共用的TLB没有清理，所以直接在TLB中读取了数据。

## 内容四：收获及感想

这次实验比想象中难度要大，主要是自己太菜了写代码老出一些小bug，而在这样大量代码的环境下debug是很耗费时间和精力的。以后要培养好的编程习惯，也可以增强debug的熟练度。

## 内容五：对课程的意见和建议

无

## 内容六：参考文献

1.  https://stackoverflow.com/questions/8843818/what-does-the-fpermissive-flag-do
2. http://blog.csdn.net/qyanqing/article/details/9381859 
3. https://wenku.baidu.com/view/47ba36d4d1f34693daef3ed7.html

