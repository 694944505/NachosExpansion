# 系统调用实习报告



[TOC]

## 内容一：总体概述

本次实验通过修改nachos源码用信号量来实现锁和条件变量，并利用它们实现同步和互斥机制应用实例，深入理解操作系统的同步机制。

## 内容二：任务完成情况

### 任务完成列表（Y/N）

|          | Exercise1 | Exercise2 |
| -------- | --------- | --------- |
| 第一部分 | Y         |           |
| 第二部分 | Y         | Y         |
| 第三部分 | Y         | Y         |

### 具体Exercise的完成情况

#### 第一部分：理解Nachos系统调用

#####Exercise1 源代码阅读

**阅读与系统调用相关的源代码，理解系统调用的实现原理。**

- code/userprog/syscall.h

  > 定义了系统调用号，声明了11个系统调用函数。

- code/userprog/exception.c

  >  定义了异常处理函数void ExceptionHandler(ExceptionType which)，根据异常类型which执行相应的处理程序（which=SyscallException时处理系统调用）。

- code/test/start.s: 

  > 辅助用户程序运行的汇编语言。
  >
  > - \_\_start函数调用了用户程序的main函数，如果main函数返回了，就调用Exit函数退出当前进程。
  > - 系统调用部分是将的调用号存放到r2寄存器中（参数放到r4、r5、r6、r7），然后通过syscall指令陷入内核，交给内核处理，处理返回值放到r2寄存器。



#### 第二部分：文件系统相关的系统调用

#####Exercise 2 系统调用实现

**类比Halt的实现，完成与文件系统相关的系统调用：Create, Open，Close，Write，Read。Syscall.h文件中有这些系统调用基本说明。**

系统调用成功后在OneInstruction中会直接return，所以要在系统调用函数中更新PC，在Machine类中增加成员函数

```
void Machine::AdvancePC()
{
    registers[PrevPCReg] = registers[PCReg]; // for debugging, in case we
                                             // are jumping into lala-land
    registers[PCReg] = registers[NextPCReg];
    registers[NextPCReg] = registers[NextPCReg] + 4;
}
```

- Create实现：从r4寄存器记录的地址中读取文件名，在调用文件系统的Create方法。

  ```
  ...
  switch (type){
  	case SC_Create:
  	{
  		int addr = machine->ReadRegister(4);
  		int pos = 0;
  		char name[FileNameMaxLen];
  		for (int i = 0; i < FileNameMaxLen; i++)
  		{
  			int data;
  			machine->ReadMem(addr + i, 1, &data);
  			name[i] = data;
  			//printf("*%c",name[i]);
  			if (data == 0){
  				//printf("---%d\n",i);
  				break;
  			}    
  		}
  		int size = 128;
  		printf("***System creat file %s with size %d.\n", name, size);
  		fileSystem->Create(name, size);
  		machine->AdvancePC();
  		break;
  	}
  	...
  }
  ```

- Open实现：和Create类似，最后将打开文件指针存入r2寄存器作为返回值。

  ```
  ...
  case SC_Open:
  {
  	int addr = machine->ReadRegister(4);
  	char name[FileNameMaxLen];
  	for (int i = 0; i < FileNameMaxLen; i++)
  	{
  		int data;
  		machine->ReadMem(addr + i, 1, &data);
  		name[i] = data;
  		if (data == 0)
  			break;
  	}
  	printf("***System open %s.\n", name);
  	OpenFile *op = fileSystem->Open(name);
  	machine->WriteRegister(2, int(op));
  	machine->AdvancePC();
  	break;
  }
  ```

- Close实现：从r4寄存器获取OpenFile指针，将其释放。

  ```
  ...
  case SC_Close:
  {
  	printf("***System close.\n");
  	OpenFile *op = (OpenFile *)machine->ReadRegister(4);
  	delete op;
  	machine->AdvancePC();
  	break;
  }
  ```

- Write实现：从r4、r5、r6寄存器读取参数，从内存中读取内容到buffer中，再调用OpenFile类的Write方法将buffer中的内容写入文件。如果输出文件是控制台，直接打印。

  ```
  ...
  case SC_Write:
  {
  	printf("***System write.\n");
  	int pos = machine->ReadRegister(4), size = machine->ReadRegister(5);
  	OpenFile *op = machine->ReadRegister(6);
  	int data;
  	char buffer[size];
  	for (int i = 0; i < size; i++)
  	{
  		machine->ReadMem(pos + i, 1, &data);
  		buffer[i] = data;
  	}
  	
  	if (machine->ReadRegister(6) == ConsoleOutput)
  	{
  		//printf("***ConsoleOutput: %s\n",buffer);
  		printf("%s\n", buffer);
  	}
  	else
  	{
  		printf("***Content: %s\n",buffer);
  		op->Write(buffer, size);
  	}
  	machine->AdvancePC();
  	break;
  }
  ```

- Read实现：从r4、r5、r6寄存器读取参数，调用OpenFile类的Read方法从文件中读取内容到buffer中，再将内容写到内存中的指定位置，将读取的字节数作为返回值写入r2寄存器。

  ```
  ...
  case SC_Read:
  {
  	printf("***System read.\n");
  	int pos = machine->ReadRegister(4), size = machine->ReadRegister(5);
  	OpenFile *op = machine->ReadRegister(6);
  	char buffer[size];
  	int readSize = op->Read(buffer, size);
  	for (int i = 0; i < readSize; i++)
  	{
  		machine->WriteMem(pos + i, 1, (int)buffer[i]);
  	}
  	machine->WriteRegister(2, readSize);
  	machine->AdvancePC();
  	break;
  }
  ```

##### Exercise 3 编写用户程序

**编写并运行用户程序，调用练习2中所写系统调用，测试其正确性。**

测试程序如下

```
#include "syscall.h"

#define Len 45
char buffer[Len];
OpenFileId fd;

int
main(){
	Create("file_syscall_test.txt");
	fd=Open("file_syscall_test.txt");
	Write("This is the content of file_syscall_test.txt",Len,fd);
	Close(fd);
	fd=Open("file_syscall_test.txt");
	Read(buffer,Len,fd);
	Write("Read result:",14,ConsoleOutput);
	Write(buffer,Len,ConsoleOutput);
}
```

测试结果如下，可以看到系统正确地建立了文件并进行了读写操作

![1](C:\Users\69494\Desktop\软微\操统\lab5\images\1.PNG)



#### 第二部分：执行用户程序相关的系统调用

##### Exercise 4 系统调用实现

**实现如下系统调用：Exec，Fork，Yield，Join，Exit。Syscall.h文件中有这些系统调用基本说明。**

- Exec实现：调用Thread类的成员函数创建一个新线程t，在execFunc中解析参数并建立t的地址空间，最后调用machine的run方法。

  ```
  ...
  case SC_Exec:
  {   
  	printf("***System exec.\n");
  	int addr=machine->ReadRegister(4);
  	Thread *t = allocThread("Exec Thread");
  	t->Fork(execFunc, (void *)addr);
  	machine->WriteRegister(2, t->getThreadID());
  	machine->AdvancePC();
  	break;
  }
  void execFunc(void *which)
  {
      int addr =which;
      int pos = 0;
      char name[FileNameMaxLen];
      for (int i = 0; i < FileNameMaxLen; i++)
      {
          int data;
          machine->ReadMem(addr + i, 1, &data);
          name[i] = data;
          if (data == 0){
              break;
          }    
      }
      OpenFile *executable = fileSystem->Open(name);
      AddrSpace *space = new AddrSpace(executable);
      delete executable;
      currentThread->space=space;
      currentThread->space->InitRegisters(); // set the initial register values
      currentThread->space->RestoreState();  // load page table register
      printf("Now run %s\n", currentThread->getName());
      
      machine->Run(); // jump to the user progam
  }
  ```

- Fork实现：调用Thread类的成员函数创建一个新线程t，在forkFunc中初始化PC为Fork的地址参数，这样他就会从地址参数所指向的函数开始运行。

  ```
  ...
  case SC_Fork:
  {
  	printf("***System fork.\n");
  	int addr=machine->ReadRegister(4);
  
  	AddrSpace *space=new AddrSpace(currentThread->space->execFile);
  	Thread *t = allocThread("Fork Thread");
  	t->space = space;
  	t->Fork(forkFunc, (void *)addr);
  	machine->AdvancePC();
  	break;
  }
  void forkFunc(void *which){
      currentThread->space->InitRegisters(); // set the initial register values
      currentThread->space->RestoreState();  // load page table register
      printf("Now run %s\n", currentThread->getName());
      machine->WriteRegister(PCReg,which);
      machine->WriteRegister(NextPCReg,which+4);
      machine->Run(); // jump to the user progam
  }
  ```

- Yield实现：直接调用currentThread->Yield()即可。

  ```
  ...
  case SC_Yield:
  {
  	printf("***System yield, current thread is %s.\n",currentThread->getName());
  	currentThread->Yield();
  	machine->AdvancePC();
  	break;
  }
  ```

- Join实现：循环判断id所对应的线程是否以及结束，如果没有结束，调用currentThread->Yield()。

  ```
  ...
  case SC_Join:
  {
  	int id=machine->ReadRegister(4);
  	printf("***System join thread %d.\n",id);
  	
  	while (!ThreadDroped(id))
  	{
  		currentThread->Yield();
  	}
  	machine->WriteRegister(2, 0);
  	machine->AdvancePC();
  	break;
  }
  ```

- Exit实现：释放内存空间，调用currentThread->Finish()。

  ```
  ...
  case SC_Exit:
  {
  #ifdef USE_TLB
  	if (!machine->multiThread)
  		printf("TLB Hit times:%d Miss times:%d Hit rate:%f\n",
  			   machine->tlbHit, machine->tlbMiss,
  			   (float)machine->tlbHit / (machine->tlbMiss + machine->tlbHit));
  #endif
  	if (currentThread->space)
  		machine->freeMemory();
  	printf("***System exit, %s finished.\n", currentThread->getName());
  	currentThread->Finish();
  	break;
  }
  ```

##### Exercise 5 编写用户程序

**编写并运行用户程序，调用练习4中所写系统调用，测试其正确性。**

测试程序如下

```
#include "syscall.h"

int f(){
	Write("fork thread function finished",30,ConsoleOutput);
	Exit(0);
}

int main(){
	Write("main thread function start",27,ConsoleOutput);
	Fork(f);
	Join(Exec("./test/fileTest"));
	Write("main thread function finished",30,ConsoleOutput);
	Yield();
	Exit(0);
}
```

测试结果如下，可以看到main中的后三条语句在exec线程完成之后才执行。

![2](C:\Users\69494\Desktop\软微\操统\lab5\images\2.PNG)

## 内容三：遇到的困难以及解决方法

1. 用switch语句处理系统调用时各个case局部变量名字一样时出现重定义错误。

   > 用{}框起来各个case下的代码。

2. 这次lab遇到了一个非常奇怪的bug，如下图，用Exec执行和fileTest一样的程序fileSyscallTest，在输出Exercise5中的结果之后fileSyscallTest程序会陷入死循环打印"d"。事实上Exercise5中一开始我是用的下图所示的测试程序，在出现bug之后我把画横线部分改为了sort来测试，发现没有问题。最后我新建了和fileSyscallTest.c内容一样fileTest.c文件，编译之后运行就没有问题了。

   ![4](C:\Users\69494\Desktop\软微\操统\lab5\images\4.png)

   > 目前不清楚为什么会有这样的bug。

## 内容四：收获及感想

这次实验的综合性比较强，完成各个exercise的过程中我也复习了前几次实验。

## 内容五：对课程的意见和建议

无

## 内容六：参考文献

1. [Nachos lab6实习报告](https://wenku.baidu.com/view/bd03f40ee97101f69e3143323968011ca300f71e.html)

2. [Nachos中文教程 - 百度文库 (baidu.com)](https://wenku.baidu.com/view/47ba36d4d1f34693daef3ed7.html)

