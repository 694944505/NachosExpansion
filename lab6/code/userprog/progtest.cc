// progtest.cc
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"
extern void Copy(char *unixFile, char *nachosFile),Print(char *name);
void threadTest(void *which)
{
    currentThread->space->InitRegisters(); // set the initial register values
    currentThread->space->RestoreState();  // load page table register
    printf("Now run %s\n", currentThread->getName());

    machine->Run(); // jump to the user progam
}
//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL)
    {
        printf("Unable to open file %s\n", filename);
        return;
    }
    space = new AddrSpace(executable);
    currentThread->space = space;
    if (machine->multiThread)
    {
        //Run the same user program agin
        AddrSpace *space;

        if (executable == NULL)
        {
            printf("Unable to open file %s\n", filename);
            return;
        }
        space = new AddrSpace(executable);
        Thread *t = allocThread("userThread2");
        t->space = space;
        t->Fork(threadTest, (void *)0);
    }
    //delete executable; // close file
    space->InitRegisters(); // set the initial register values
    space->RestoreState();  // load page table register

    machine->Run(); // jump to the user progam

    ASSERT(FALSE); // machine->Run never returns;
                   // the address space exits
                   // by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void ConsoleTest(char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);

    for (;;)
    {
        readAvail->P(); // wait for character to arrive
        ch = console->GetChar();
        console->PutChar(ch); // echo it!
        writeDone->P();       // wait for write to finish
        if (ch == 'q')
            return; // if q, quit
    }
}
void execThread(void *which)
{
    char *filename=(char *) which;
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL)
    {
        printf("Unable to open file %s\n", filename);
        return;
    }
    printf("open file %s\n", filename);
    space = new AddrSpace(executable);
    currentThread->space = space;
    //delete executable; // close file
    space->InitRegisters(); // set the initial register values
    space->RestoreState();  // load page table register

    machine->Run(); // jump to the user progam
}

char Usage[500]="usage:\n  help\t Print usage\n  quit\t Quit\n  ls[path]\t Display the content in \"path\"\n  cd<path>\t Enter \"path\"\n  mkdir<directory>\t Make \"directory\"\n  rm<file>\t Delete \"file\"\n  create<file>\t Make \"file\"\n  cat<file>\t Display the content of \"file\"\n  cp<file1><file2>\t Copy \"file1\" to \"file2\"\n  exec<file>\t Execute \"file\"\n\n";
char Shell[20]="shell:";
void printUsage(){
    printf("usage:\n");
    printf("  help\t Print usage\n");
    printf("  quit\t Quit\n");
    printf("  ls[path]\t Display the content in \"path\"\n");
    printf("  cd<path>\t Enter \"path\"\n");
    printf("  mkdir<directory>\t Make \"directory\"\n");
    printf("  rm<file>\t Delete \"file\"\n");
    printf("  create<file>\t Make \"file\"\n");
    printf("  cat<file>\t Display the content of \"file\"\n");
    printf("  cp<file1><file2>\t Copy \"file1\" to \"file2\"\n");
    printf("  exec<file>\t Execute \"file\"\n");
    printf("\n");
}
void printOnConsole(SynchConsole *console,char *out){
    for(int i=0;out[i];i++){
        console->PutChar(out[i]); 
    }
}
static SynchConsole *synchConsole;

void SynchConsoleTest(char *in, char *out)
{
    char ch;
    char arg[3][100];
    int i=0,j=0;
    synchConsole = new SynchConsole(in, out);
    printOnConsole(synchConsole,Shell);
    //printf("%s",Shell);
    for(;;){
        while(TRUE){
            //scanf("%c",&ch);
            //printf("%c",ch);
            ch = synchConsole->GetChar();
            //synchConsole->PutChar(ch);
            if(ch=='\n'){
                
                arg[i][j]=0;
                if(i==0){
                    if(strcmp(arg[0],"quit")==0) {
                        interrupt->Halt();
                    }
                    else if(strcmp(arg[0],"help")==0) printOnConsole(synchConsole,Usage);
                    else if(strcmp(arg[0],"ls")==0) fileSystem->List(".");
                    else {
                        //printf("function not support!\n");
                        ASSERT(FALSE);
                    }
                }else if(strcmp(arg[0],"ls")==0){
                    ASSERT(i==1);
                    fileSystem->List(arg[1]);
                }else if(strcmp(arg[0],"cd")==0){
                    ASSERT(i==1);
                    if(!fileSystem->Enter(arg[1])){
                        ;//printf("Error: %s not found\n",arg[1]);
                    }
                }else if(strcmp(arg[0],"mkdir")==0){
                    ASSERT(i==1);
                    if(!fileSystem->Mkdir(arg[1])){
                        ;//printf("Error: %s not found\n",arg[1]);
                    }
                }else if(strcmp(arg[0],"rm")==0){
                    ASSERT(i==1);
                    if(!fileSystem->Remove(arg[1])){
                        ;//printf("Error: %s not found\n",arg[1]);
                    }
                }else if(strcmp(arg[0],"create")==0){
                    ASSERT(i==1);
                    if(!fileSystem->Create(arg[1],0)){
                        ;//printf("Error: %s not found\n",arg[1]);
                    }
                }else if(strcmp(arg[0],"cat")==0){
                    ASSERT(i==1);
                    Print(arg[1]);
                }else if(strcmp(arg[0],"cp")==0){
                    ASSERT(i==2);
                    Copy(arg[1],arg[2]);
                }else if(strcmp(arg[0],"exec")==0){
                    ASSERT(i==1);
                    Thread *t = allocThread("Exec Thread");
                    t->Fork(execThread, (void *)arg[1]);
                    currentThread->Yield();
                }else{
                    ;//printf("function not support!\n");
                    ASSERT(FALSE);
                }
                i=j=0;
                printOnConsole(synchConsole,Shell);
                //printf("shell:");
                break;
            }
            if(ch!=' ') arg[i][j++]=ch;
            else {
                arg[i++][j]=0;
                j=0;
            }
        }
    }
    
}