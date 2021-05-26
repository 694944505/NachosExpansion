# 线程机制和线程调度实习报告


[TOC]



##内容一：总体概述


本次lab通过修改Nachos系统平台的底层源代码，实现对线程机制和线程调度的扩展。主要考察了对进程控制块（PCB）数据结构、线程调度算法以及Nachos源码组织结构的理解。

##内容二：任务完成情况
###任务完成列表
|Linux|PCB实现方式|调度算法|
|-|-|-|
| 调研     | Y         | Y         |

|          |Exercise1|Exercise2|Exercise3|Exercise4|Exercise5|Challenge|
|-         |-        |-        |-        |-        |-        |-        |
| 实习内容  | Y         | Y         | Y         | Y         | Y         | Y         |

###具体Exercise的完成情况
####调研
#####调研Linux或Windows中进程控制块（PCB）的基本实现方式，理解与Nachos的异同。

我调研了Linux系统，它每一个进程由一个task\_struct数据结构记录，其主要内容有

- 进程标识
> 包括进程标识符、用户标识符、组标识符、备份用户标识符、文件系统用户标识符等。
- 调度信息
> 包括进程状态、优先级、调度策略等。
- 时间数据成员
> 包括定时器信息、进程的睡眠时间、消耗时间等。
- 进程上下文环境
> 包括寄存器、堆栈的值等
- 文件系统数据成员
> 记录打开的文件列表等
- 内存数据成员
> 记录虚拟地址内存空间的现状
- 页面管理
- 进程通信相关信息
- 进程队列指针
- 其它数据成员

相对于Linux来说，Nachos的PCB结构比较简单，仅定义了几个必须的变量。

#####调研Linux或Windows中采用的进程/线程调度算法。

Linux内核将进程分为实时进程和普通进程，实时进程的优先级高于普通进程。

Linux实现了6种不同的调度策略对不同类型的进程进行调度

|字段	|描述	|
|-|-|-|
|SCHED_NORMAL	|也叫SCHED_OTHER，是一种分时调度策略，用于普通进程|
|SCHED_BATCH|SCHED_NORMAL普通进程策略的分化版本，根据动态优先级分配CPU运算资源。适合于成批处理的工作	CFS|
|SCHED_IDLE|优先级最低，在系统空闲时才 跑|
|SCHED_FIFO|先入先出调度算法（实时进程调度策略），相同优先级的任务先到先服务，高优先级的任务可以抢占低优先级的任务	|
|SCHED_RR|时间片轮转调度算法（实时进程调度策略）|
|SCHED_DEADLINE|针对突发型计算（实时进程调度策略）|

####实习内容
#####Exercise 1 源代码阅读
**仔细阅读下列源代码，理解Nachos现有的线程机制。**

    code/threads/main.cc和code/threads/threadtest.cc
    
    code/threads/thread.h和code/threads/thread.cc

- main.cc
> 是整个Nachos系统的入口，它对Nachos的命令行参数进行分析并进行不同功能的初始化，然后调用操作系统的方法。
- threadtest.cc
> 用于编写测试用例，通过testnum设置不同的测试函数。源码自带的测试函数内容是Fork两个线程，交替调用Yield()主动放弃CPU。
- thread.h和thread.cc
> 定义了Thread类，即Nachos的PCB结构。主要包括了线程状态、堆栈指针、寄存器状态。实现了以下方法
    1. void Fork(VoidFunctionPtr func, void *arg):
      初始化线程，将其放入就绪队列。
    2. void Finish():
      将threadToBeDestroyed的值设为当前线程，待scheduler调用销毁程序，然后调用Sleep()方法。
    3. void Yield():
      将当前线程放到就绪队列队尾，然后调用scheduler的方法找到就绪队列中的下一个线程，并让其执行。
    4. void Sleep():
      将当前线程置阻塞态，运行就绪队列的下一个线程（如果就绪队列为空则调用scheduler的Idle方法）。

#####Exercise 2 扩展线程的数据结构
**增加“用户ID、线程ID”两个数据成员，并在Nachos现有的线程管理机制中增加对这两个数据成员的维护机制。**

在`Thread`类中增加`threadID`和`userID`两个字段，`threadID`从0开始增加，`userID`调用`unistd.h`头文件下的`getuid()`方法初始化。

用`ThreadTest2`函数测试，可以看到在不同用户执行程序thread得到了不同的用户id

    void emptyThread(int which) {}
    
    //----------------------------------------------------------------------
    // ThreadTest2
    // print threadInfo
    //----------------------------------------------------------------------
    //extern Thread threads[];
    void ThreadTest2()
    {
        DEBUG('t', "Entering ThreadTest2");
    
        Thread *t = allocThread("forked thread");
        t->Fork(emptyThread, (void *)0);
        t->printInfo();
    }
![](images/1.png)
#####Exercise 3 增加全局线程管理机制
**在Nachos中增加对线程数量的限制，使得Nachos中最多能够同时存在128个线程；
仿照Linux中PS命令，增加一个功能TS(Threads Status)，能够显示当前系统中所有线程的信息和状态。**
在`thread.h`中添加宏定义`#define ThreadNum 128`。仿照xv6的源码，建立一个线程池，在`thread.cc`中用数组`Thread threads[ThreadNum]`保存。修改源码中调用Thread类的构造函数和析构函数的部分，分别改成从线程池中选取未使用的槽位和调用`endThread()`Thread类成员方法释放申请的空间。在从线程池中搜索时，如果所有槽位都被占了就调用`ASSERT(0)`方法退出程序。TS功能可以通过遍历打印线程池中被占用的槽位信息的方法实现。使用`TestThread3()`函数测试如下

    void ThreadTest3()
    {
        DEBUG('t', "Entering ThreadTest3");
        Thread *t = allocThread("forked thread");
        t->Fork(emptyThread, (void *)0);
        currentThread->Yield();
        TS();
        for (int i = 0; i < 130; i++)
        {
            Thread *t = allocThread("forked thread");
            t->Fork(emptyThread, (void *)0);
            printf("***succussfully forked thread with threadID: %d\n", t->getThreadID());
        }
    }

![](images/2.png)
![](images/3.png)

#####Exercise 4  源代码阅读
**仔细阅读下列源代码，理解Nachos现有的线程调度算法。**

    code/threads/scheduler.h和code/threads/scheduler.cc
    code/threads/switch.s
    code/machine/timer.h和code/machine/timer.cc

- scheduler.h和scheduler.cc
> 定义了nachos中的进程调度器，以链表readyList保存就绪的进程队列。实现了以下接口
    1. void ReadyToRun(Thread* thread):
      将thread的状态设置为就绪，并放到readyList中
    2. Thread* FindNextToRun(int source):
      从就绪队列中取出下一个要运行的线程
    3. void Run(Thread* nextThread):
      保存当前的机器信息，然后调用SWITCH方法运行nextThread
- switch.s
> 内容是汇编代码，负责CPU上进程的切换。
切换过程中，首先保存当前进程的状态，然后恢复新运行进程的状态，之后切换到新进程的地址空间，开始运行新进程。
- timer.h和timer.cc
> 定义了Timer类，用于模拟硬件上的时间中断，实现了以下方法：
    1. 构造函数Timer(VoidFunctionPtr timerHandler, int callArg, bool doRandom):
      timerHandler和callArg是处理中断的函数和其参数，doRandom记录是否允许随机中断事件。初始化参数然后在中断队列中放入第一个时钟中断
    2. void TimerExpired():
      在中断队列中放入下一个时钟中断，然后调用当前中断的处理函数
    3. int TimeOfNextInterrupt():
      返回下一个时钟中断的间隔

#####Exercise 5  线程调度算法扩展
**扩展线程调度算法，实现基于优先级的抢占式调度算法。**
在`Thread`类中添加表示优先级的成员变量`priority`。开启时钟中断，改写`system.cc`中的时钟中断处理函数`TimerInterruptHandler()`，在当前线程优先级低于就绪队列中的最高优先级时，调用`interrupt`的`YieldOnReturn()`方法，实现线程切换。使用`TestThread5()`函数测试如下

    void priorityThread(int which)
    {
        currentThread->printInfo();
        if (which >= 0)
        {
            Thread *t = allocThread("forked thread", which);
            t->Fork(priorityThread, (void *)which - 3);
        }
        currentThread->printInfo();
        currentThread->printInfo();
    }
    //----------------------------------------------------------------------
    // ThreadTest5
    // priority test
    //----------------------------------------------------------------------
    void ThreadTest5()
    {
        DEBUG('t', "Entering ThreadTest5");
        priorityThread(PriorityRange - 1);
    }


![](images/4.png)
#####Challenge 线程调度算法扩展
**可实现“时间片轮转算法”、“多级队列反馈调度算法”，或将Linux或Windows采用的调度算法应用到Nachos上。**
本次lab实现了时间片轮转算法。在`Thread`类中添加记录线程上CPU的时刻的成员变量`lastStartTime`和记录分配的时间片的成员变量`timeSlice`，修改`Scheduler`类的`Run()`方法，在调用`SWICH()`方法前设置好线程的`lastStartTime`。改写`system.cc`中的时钟中断处理函数`TimerInterruptHandler()`，在线程的时间片用完时调用`interrupt`的`YieldOnReturn()`方法，实现线程切换。使用`TestThread6()`函数测试如下

    void RRThread(int which)
    {
        for (int i = 0; i < which; i++)
        {
            printf("*** ThreadID: %d, ThreadPriority: %d, CurrentTime: %d, StartTime: %d, TimeUsed:%d, TimeSlice: %d\n",
                currentThread->getThreadID(), currentThread->getPrioriry(), stats->totalTicks, currentThread->getStartTime(), stats->totalTicks - currentThread->getStartTime(), currentThread->getTimeSlice());
            interrupt->SetLevel(IntOff);
            interrupt->SetLevel(IntOn);
        }
    }
    //----------------------------------------------------------------------
    // ThreadTest6
    // RR test
    //----------------------------------------------------------------------
    void ThreadTest6()
    {
        DEBUG('t', "Entering ThreadTest6");
    
        // currentThread->setTimeSlice(20);
        for (int i = 0; i < 2; i++)
        {
            Thread *t = allocThread("forked thread");
            t->setTimeSlice(20);
            t->Fork(RRThread, (void *)3);
        }
        currentThread->Yield();
    }


![](images/5.png)

##遇到的困难及解决办法
1. 对nachos源码了解不够多，在修改时常遇到与预期不符的情况

   > 使用-d参数运行程序，通过源码中的`Debug`方法查看每一步运行的信息，与自己的思路对比。

2. 在建立线程池后运行程序发现析构函数的`Assertion failed`
  ![](images/6.png)

  > 原因是线程池的`Thread`数组在系统结束运行之后才调用析构函数，在`Statistics`类中添加`halted`变量，系统结束后`halted`置为`1`，在`ASSERT`语句中添加系统是否结束的判断。

3. 补充一下，助教老师在习题课上提到当前nachos的回收线程的机制是有问题的。因为如果新线程上CPU后调用Finish下CPU，它是执行不到delete threadToBeDestroyed语句的，这样上一个threadToBeDestroyed可能就会在没有delete掉的情况下被这个新线程直接覆盖，从而本来threadToBeDestroyed指向的这个线程不会被delete掉。用以下函数测试

   ```c++
   void finishThread(int which){
       printf("Finshing thread \"%s\"\n",currentThread->getName());
       currentThread->Finish();
   }
   ```

   ```c++
   void
   ThreadTest7()
   {
       DEBUG('t', "Entering ThreadTest2");
       
   
       Thread *t1 = new Thread("forked thread 1");
       
       t1->Fork(finishThread, (void*)0);
       
       Thread *t2 = new Thread("forked thread 2");
       
       t2->Fork(finishThread, (void*)0);
       
       currentThread->Yield();
   
   }
   ```

   结果如下，可以看到运行过程中"forked thread 1"并没有被回收掉

   ![5](C:\Users\69494\Pictures\5.PNG)

   > 在Thread类中添加变量droped，初始化为0在Thread类的Finish成员函数中将droped置为1。修改Scher类的Run函数，在SWICH前用循环语句回收线程池里不用的线程。测试结果如下
   >
   > ![6](C:\Users\69494\Pictures\6.PNG)

##收获及感想

在阅读源码时前期的工作量是比较大的，因为在运行过程中遇到的问题可能涉及到各个模块，这就需要对源码耐心的阅读与理解。在阅读nachos源码时体会到了它良好的可阅读性和可扩展性，其设计思路值得借鉴。在修改源码过程中遇到了一些bug，在debug之后没有及时记录，到写报告的时候就忘记了，以后可以注意一下。
在实现调度算法时自由发挥的空间是比较大的，最终选择在时钟中断函数中实现，这样方便各个算法的统一，但也把时钟中断的周期设置得比较短，有待进一步调研。

##对课程的意见和建议
无

##参考文献
1. https://wenku.baidu.com/view/47ba36d4d1f34693daef3ed7.html

2. https://github.com/ranxian/xv6-chinese/tree/master/content