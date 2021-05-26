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

    default:
        printf("No test specified.\n");
        break;
    }
}
