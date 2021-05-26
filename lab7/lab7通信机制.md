# 通信机制实习报告



[TOC]

## 内容一：总体概述

调研Linux中进程通信的机制，并在Nachos上实现一种。

## 内容二：任务完成情况

### 任务完成列表（Y/N）

|          | Exercise1 | Exercise2 | **Exercise3** |
| -------- | :-------- | --------- | ------------- |
| 实习内容 | Y         | Y         | Y             |

### 具体Exercise的完成情况

#### 实习内容

#####Exercise1 

**调研Linux中进程通信机制的实现**

Linux下进程间通信的几种主要手段：

- 管道(Pipe)及有名管道(named pipe),具有以下特点：
  - 数据只能向一个方向流动，需要双方通信时,需要建立起两个管道

  - 管道只能用于父子进程或者兄弟进程之间(具有亲缘关系的进程)

  - 数据的读出和写入：一个进程向管道中写的内容被管道另一端的进程读出。写入的内容每次都添加在管道缓冲区的末尾，并且每次都是从缓冲区的头部读出数据。

  - 有名管道以FIFO文件的形式出现在文件系统中，所以任何进程都可以通过使用其文件名来打开管道，然后进行读写。
- 信号（Signal）：信号可用于进程间通信，也用于内核与进程之间的通信（内核只能向进程发送信号而不能接收）。发送信号时需要用到对方的pid，而一般只有父子进程才知道对方的pid，所以实际上还是只能用于父子进程。
- 报文（Message）队列（消息队列）：消息队列是内核地址空间中的内部链表，通过linux内核在各个进程直接传递内容，消息顺序地发送到消息队列中，并以几种不同的方式从队列中获得，每个消息队列可以用 IPC标识符唯一地进行识别。消息队列克服了信号承载信息量少，管道只能承载无格式字节流以及缓冲区大小受限等缺点。
- 信号量（Semaphore）：信号量是一种计数器，用于控制对多个进程共享的资源进行的访问。
- 共享内存（Share Memory）：使得多个进程可以访问同一块内存空间，是最快的可用IPC形式。是针对其他通信机制运行效率较低而设计的。往往与其它通信机制，如信号量结合使用，来达到进程间的同步及互斥。
- 套接口（Socket）：更为一般的进程间通信机制，可用于不同机器之间的进程间通信。起初是由Unix系统的BSD分支开发出来的，但现在一般可以移植到其它类Unix系统上：Linux和System V的变种都支持套接字。

#####Exercise2 为Nachos设计并实现一种线程/进程间通信机制

**基于已完善的线程、调度、同步机制部分，选择Linux中的一种进程通信机制，在Nachos上实现。**

本次实验选择消息队列机制，nachos自身已经在threads/synchlist.h中设计了一个SynchList类，它维护了一个资源队列，主要接口如下

> void Append(void *item)：将item插入队尾，唤醒等待链表元素的进程。
>
> void *Remove()：从队头取出一个元素，如果队列为空就等待。

利用SynchList可以很方便的实现消息队列。首先为每个线程对象绑定一个SynchList类的对象

```
Thread threads[ThreadNum];
SynchList messageList[ThreadNum];
```

在线程类中添加发送和接受消息的成员函数，实现消息队列

```
void Thread::sendMessage(void *message ,int dst){
    for(int i=0;i<ThreadNum;i++){
        if(threads[i].getThreadID()==dst&&threads[i].getUsed()&&!threads[i].droped){
            messageList[i].Append(message);
            return ;
        }
    }
    printf("Thread %d not exist\n",dst);
}
void *Thread::receiveMessage(){
    return messageList[this-threads].Remove();
}
```

#####Exercise3

**为实现的通信机制编写测试用例**

在userprog/progtest.cc中编写测试函数如下

```
void receiveThread(int which){
    char *message=(char *)currentThread->receiveMessage();
    printf("***%s receive: %s\n",currentThread->getName(),message);
}

void MessageTest(){
    Thread *t1=allocThread("r1"),*t2=allocThread("r2");
    t1->Fork(receiveThread,0);
    t2->Fork(receiveThread,0);
    currentThread->Yield();
    char message[10]="hello";
    printf("***%s send to %s : %s\n",currentThread->getName(),t1->getName(),message);
    currentThread->sendMessage(message,t1->getThreadID());
    printf("***%s send to %s : %s\n",currentThread->getName(),t2->getName(),message);
    currentThread->sendMessage(message,t2->getThreadID());
}
```

结果

![1](C:\Users\69494\Desktop\软微\操统\lab7\images\1.PNG)

## 内容三：遇到的困难以及解决方法

1. 这次实验在lab6基础上做的，编译时发现在/code目录下执行make命令会报错（lab6中是在/code/filesys目录下执行的make depend和make nachos命令，编译不报错）

   ![2](C:\Users\69494\Desktop\软微\操统\lab7\images\2.PNG)
   
   > 感觉是Makfile的问题，但没改好，于是把Print和Copy在progtest.cc中重写了一遍。


## 内容四：收获及感想

这次实验相对比较简单，消息队列和生产者-消费者问题是类似的。

## 内容五：对课程的意见和建议

无

## 内容六：参考文献

1. [Nachos中文教程 - 百度文库 (baidu.com)](https://wenku.baidu.com/view/47ba36d4d1f34693daef3ed7.html)

   