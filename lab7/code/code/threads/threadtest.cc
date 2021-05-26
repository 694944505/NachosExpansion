// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread(int which)
{
    int num;
    //currentThread->Print();
    for (num = 0; num < 5; num++)
    {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    //Thread *t = new Thread("forked thread");
    Thread *t = allocThread("forked thread");
    t->Fork(SimpleThread, (void *)1);
    SimpleThread(0);
}

// functions for PKU tasks

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

//----------------------------------------------------------------------
// ThreadTest3
// fork threads until the number of threads reached the limit
//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
// ThreadTest4
//
//----------------------------------------------------------------------
void ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");
    Thread *t = allocThread("forked thread");
    t->Fork(emptyThread, (void *)0);
    for (int i = 0; i < 10; i++)
        printf("DEBUG %d %d\n", i, stats->totalTicks);
    printf("---------------------------------------\n");
}

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

void finishThread(int which){
    printf("Finshing thread \"%s\"\n",currentThread->getName());
    currentThread->Finish();
}

void
ThreadTest7()
{
    DEBUG('t', "Entering ThreadTest2");
    
    Thread *t1 = allocThread("forked thread 1");

    t1->Fork(finishThread, (void*)0);

    Thread *t2 = allocThread("forked thread 2");

    t2->Fork(finishThread, (void*)0);

    currentThread->Yield();

}

int products,N;
Semaphore *empty,*full,*mutex;
void producerUsingSemaphore(int which){
    for(int i=0;i<which;i++){
        //produce
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);

        empty->P();
        mutex->P();
        products++;
        printf("***%s produced an item. now there are %d products!\n",
        currentThread->getName(),products);
        mutex->V();
        full->V();
    }
    printf("***%s has done its job.\n",currentThread->getName());
}
void consumerUsingSemaphore(int which){
    for(int i=0;i<which;i++){
        full->P();
        mutex->P();
        products--;
        printf("***%s consumed an item. now there are %d products!\n",
        currentThread->getName(),products);
        mutex->V();
        empty->V();
        //consume
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
        
    }
    printf("***%s has done its job.\n",currentThread->getName());
}
//----------------------------------------------------------------------
// ThreadTest8
// solve producer-consumer problem using semaphores
//----------------------------------------------------------------------
void ThreadTest8(){
    products=0;
    N=3;
    empty=new Semaphore("empty",N);
    full=new Semaphore("full",0);
    mutex=new Semaphore("mutex",1);
    Thread *comsumer1=allocThread("comsumer1");
    comsumer1->Fork(consumerUsingSemaphore,(void*)(2*N-1));
    Thread *comsumer2=allocThread("comsumer2");
    comsumer2->Fork(consumerUsingSemaphore,(void*)(2*N));
    Thread *producer1=allocThread("producer1");
    producer1->Fork(producerUsingSemaphore,(void*)(2*N));
    Thread *producer2=allocThread("producer2");
    producer2->Fork(producerUsingSemaphore,(void*)(2*N-1));
    currentThread->Yield();
    
}

Lock *lock;
Condition *condition;

void producerUsingCondition(int which){
    for(int i=0;i<which;i++){
        //produce
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);

        lock->Acquire();
        while(products>=N){
            printf("***Buffer full! %s wait.\n",currentThread->getName());
            condition->Wait(lock);
        }
        products++;
        printf("***%s produced an item. now there are %d products!\n",
        currentThread->getName(),products);
        //condition->Broadcast(lock);
        condition->Signal(lock);
        lock->Release();
    }
    printf("***%s has done its job.\n",currentThread->getName());
}
void consumerUsingCondition(int which){
    for(int i=0;i<which;i++){
        lock->Acquire();
        while(products<=0){
            printf("***Buffer empty! %s wait.\n",currentThread->getName());
            condition->Wait(lock);
        }
        products--;
        printf("***%s consumed an item. now there are %d products!\n",
        currentThread->getName(),products);
        //condition->Broadcast(lock);
        condition->Signal(lock);
        lock->Release();
        //consume
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
    printf("***%s has done its job.\n",currentThread->getName());
}
//----------------------------------------------------------------------
// ThreadTest9
// solve producer-consumer problem using conditions
//----------------------------------------------------------------------
void ThreadTest9(){
    products=0;
    N=3;
    lock=new Lock("buffer lock");
    condition=new Condition("buffer lock condition");
    Thread *comsumer1=allocThread("comsumer1");
    comsumer1->Fork(consumerUsingCondition,(void*)(2*N-1));
    Thread *comsumer2=allocThread("comsumer2");
    comsumer2->Fork(consumerUsingCondition,(void*)(2*N));
    Thread *producer1=allocThread("producer1");
    producer1->Fork(producerUsingCondition,(void*)(2*N));
    Thread *producer2=allocThread("producer2");
    producer2->Fork(producerUsingCondition,(void*)(2*N-1));
    currentThread->Yield();
    
}

Lock *barrierLock;
Condition *barrierCondition;
int barrierCount,barrierThreshold;
void barrierThread(int which){
    //for(int i=0;i<which;i++){
        barrierLock->Acquire();
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
        barrierCount++;
        printf("***%s%d reached. %d threads left.",
            currentThread->getName(),currentThread->getThreadID(),barrierThreshold-barrierCount);
        if(barrierCount==barrierThreshold){
            printf("Broadcast.\n");
            barrierCondition->Broadcast(barrierLock);
            barrierLock->Release();
        }else{
            printf("Waiting for others.\n");
            barrierCondition->Wait(barrierLock);
            barrierLock->Release();
        }
        printf("***%s%d continue to run.\n",
        currentThread->getName(),currentThread->getThreadID());
    //}
}
//----------------------------------------------------------------------
// ThreadTest10
// implement barrier using semaphores
//----------------------------------------------------------------------
void ThreadTest10(){
    barrierThreshold=3;
    barrierCount=0;
    barrierLock=new Lock("barrier");
    barrierCondition=new Condition("barrier");
    for(int i=0;i<barrierThreshold;i++){
        Thread *t=allocThread("barrierThread");
        t->Fork(barrierThread,(void *)0);
    }
    //currentThread->Yield();
}

int readerCount,content;
Semaphore *rSemaphore,*rwSemaphore;
void readerThread(int which){
    for(int i=0;i<which;i++){
        rSemaphore->P();
        readerCount++;
        if(readerCount==1) rwSemaphore->P();
        rSemaphore->V();
        
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
        printf("***%s read.Content: %d\n",
        currentThread->getName(),content);
        currentThread->Yield();

        rSemaphore->P();
        readerCount--;
        if(readerCount==0) rwSemaphore->V();
        rSemaphore->V();
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}

void writerThread(int which){
    for(int i=0;i<which;i++){
        rwSemaphore->P();
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
        content++;
        printf("***%s write.Content: %d\n",
        currentThread->getName(),content);
        //currentThread->Yield();

        rwSemaphore->V();
        interrupt->SetLevel(IntOff);
        interrupt->SetLevel(IntOn);
    }
}
//----------------------------------------------------------------------
// ThreadTest11
// solve reader-writer problem using semaphores
//----------------------------------------------------------------------
void ThreadTest11(){
    readerCount=0;
    content=0;
    rSemaphore=new Semaphore("reader",1);
    rwSemaphore=new Semaphore("reader-writer",1);
    Thread *reader1=allocThread("reader1");
    Thread *reader2=allocThread("reader2");
    reader1->Fork(readerThread,(void *)3);
    reader2->Fork(readerThread,(void *)3);
    
    Thread *writer1=allocThread("writer1");
    Thread *writer2=allocThread("writer2");
    writer1->Fork(writerThread,(void *)3);
    writer2->Fork(writerThread,(void *)3);
    currentThread->Yield();
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void ThreadTest()
{
    //testnum=6;
    switch (testnum)
    {
    case 1:
        ThreadTest1();
        break;

    case 2:
        ThreadTest2();
        break;

    case 3:
        ThreadTest3();
        break;

    case 4:
        ThreadTest4();
        break;

    case 5:
        ThreadTest5();
        break;

    case 6:
        ThreadTest6();
        break;

    case 7:
        ThreadTest7();
        break;

    case 8:
        ThreadTest8();
        break;

    case 9:
        ThreadTest9();
        break;

    case 10:
        ThreadTest10();
        break;

    case 11:
        ThreadTest11();
        break;

    default:
        printf("No test specified.\n");
        break;
    }
}
