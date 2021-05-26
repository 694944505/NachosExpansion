// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "directory.h"
#include "openfile.h"
#include "addrspace.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------
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

void forkFunc(void *which){
    currentThread->space->InitRegisters(); // set the initial register values
    currentThread->space->RestoreState();  // load page table register
    printf("Now run %s\n", currentThread->getName());
    machine->WriteRegister(PCReg,which);
    machine->WriteRegister(NextPCReg,which+4);
    machine->Run(); // jump to the user progam
}

void ExceptionHandler(ExceptionType which)
{

    int type = machine->ReadRegister(2);

    if (which == PageFaultException)
    {
        //printf("%d\n",machine->ReadRegister(BadVAddrReg));
        int vpn = (unsigned)machine->ReadRegister(BadVAddrReg) / PageSize;

        currentThread->space->getPTE(vpn);
#ifdef USE_TLB
        machine->SwapTLB(vpn);
#endif
        return;
    }
    if ((which == SyscallException))
    {
        switch (type)
        {
        case SC_Halt:
        {
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;
        }

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

        case SC_Close:
        {
            printf("***System close.\n");
            OpenFile *op = (OpenFile *)machine->ReadRegister(4);
            delete op;
            machine->AdvancePC();
            break;
        }

        case SC_Write:
        {
            printf("***System write.\n");
            int pos = machine->ReadRegister(4), size = machine->ReadRegister(5);
            OpenFile *op = machine->ReadRegister(6);
            int data;
            char buffer[size+1];
            for (int i = 0; i < size; i++)
            {
                machine->ReadMem(pos + i, 1, &data);
                buffer[i] = data;
            }
            buffer[size]=0;
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
        case SC_Read:
        {
            printf("***System read.\n");
            int pos = machine->ReadRegister(4), size = machine->ReadRegister(5);
            OpenFile *op = machine->ReadRegister(6);
            char buffer[size+1];
            int readSize = op->Read(buffer, size);
            for (int i = 0; i < readSize; i++)
            {
                machine->WriteMem(pos + i, 1, (int)buffer[i]);
            }
            machine->WriteRegister(2, readSize);
            machine->AdvancePC();
            break;
        }
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
        case SC_Yield:
        {
            printf("***System yield, current thread is %s.\n",currentThread->getName());
            currentThread->Yield();
            machine->AdvancePC();
            break;
        }
        default:
            break;
        }
        return;
    }

    printf("Unexpected user mode exception %d %d\n", which, type);
    ASSERT(FALSE);
}