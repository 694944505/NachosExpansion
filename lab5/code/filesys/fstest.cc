// fstest.cc
//	Simple test routines for the file system.
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 10 // make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile *openFile;
    int amountRead, fileLength;
    char *buffer;

    // Open UNIX file
    if ((fp = fopen(from, "r")) == NULL)
    {
        printf("Copy: couldn't open input file %s\n", from);
        return;
    }

    // Figure out length of UNIX file
    fseek(fp, 0, 2);
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

    // Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength))
    { // Create Nachos file
        printf("Copy: couldn't create output file %s\n", to);
        fclose(fp);
        return;
    }

    openFile = fileSystem->Open(to);
    DEBUG('f', "--open %d \n", openFile->oSector);
    ASSERT(openFile != NULL);

    // Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
    {
        //printf("----W len%d\n",amountRead);
        openFile->Write(buffer, amountRead);
    }
    delete[] buffer;

    // Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void Print(char *name)
{
    OpenFile *openFile;
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL)
    {
        printf("Print: unable to open file %s\n", name);
        return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
        for (i = 0; i < amountRead; i++)
            printf("%c", buffer[i]);
    delete[] buffer;

    delete openFile; // close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName "TestFile"
#define Contents "1234567890"
#define ContentSize strlen(Contents)
#define FileSize ((int)(ContentSize * 10))

static void
FileWrite()
{
    OpenFile *openFile;
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n",
           FileSize, ContentSize);
    if (!fileSystem->Create(FileName, 0))
    {
        printf("Perf test: can't create %s\n", FileName);
        return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("Perf test: unable to open %s\n", FileName);
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize)
    {
        numBytes = openFile->Write(Contents, ContentSize);
        if (numBytes < 10)
        {
            printf("Perf test: unable to write %s\n", FileName);
            delete openFile;
            return;
        }
    }
    delete openFile; // close file
}

static void
FileRead()
{
    OpenFile *openFile;
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n",
           FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL)
    {
        printf("Perf test: unable to open file %s\n", FileName);
        delete[] buffer;
        return;
    }
    for (i = 0; i < FileSize; i += ContentSize)
    {
        numBytes = openFile->Read(buffer, ContentSize);
        if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize))
        {
            printf("Perf test: unable to read %s\n", FileName);
            delete openFile;
            delete[] buffer;
            return;
        }
    }
    delete[] buffer;
    delete openFile; // close file
}

void PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();
    FileWrite();
    FileRead();
    if (!fileSystem->Remove(FileName))
    {
        printf("Perf test: unable to remove %s\n", FileName);
        return;
    }
    stats->Print();
}

void write()
{
    printf("%s start\n", currentThread->getName());
    OpenFile *openFile;
    int i, numBytes;

    if (!fileSystem->Create(FileName, 0))
    {
        printf("Perf test: can't create %s\n", FileName);
        return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL)
    {
        printf("Perf test: unable to open %s\n", FileName);
        return;
    }
    currentThread->Yield();
    printf("begin writing\n");
    numBytes = openFile->Write(Contents, ContentSize);
    printf("end writing\n");
    delete openFile; // close file
}

void read(int which)
{
    printf("%s start\n", currentThread->getName());
    OpenFile *openFile;
    char *buffer = new char[ContentSize + 1];
    int i, numBytes;

    if ((openFile = fileSystem->Open(FileName)) == NULL)
    {
        printf("Perf test: unable to open file %s\n", FileName);
        delete[] buffer;
        return;
    }
    printf("begin reading\n");
    printf("%s's size is %d\n", FileName, openFile->Length());
    numBytes = openFile->Read(buffer, ContentSize);
    printf("read %d bytes\n", numBytes);
    buffer[ContentSize] = '\0';
    printf("read content : %s\n", buffer);
    printf("end reading\n");
    delete[] buffer;
    delete openFile; // close file

    if (!fileSystem->Remove(FileName))
    {
        printf("Perf test: unable to remove %s\n", FileName);
        return;
    }
}

void PerformanceTest1()
{
    printf("Starting file system performance test:\n");
    Thread *reader = allocThread("reader");
    reader->Fork(read, 0);
    write();
}

void remove(int which)
{
    printf("%s want to remove file %s\n", currentThread->getName(), FileName);
    fileSystem->Remove(FileName);
}

void PerformanceTest2()
{
    printf("Starting file system performance test:\n");

    OpenFile *openFile;
    int i, numBytes;

    if (!fileSystem->Create(FileName, 0))
    {
        printf("Perf test: can't create %s\n", FileName);
        return;
    }
    openFile = fileSystem->Open(FileName);
    printf("%s using file %s\n", currentThread->getName(), FileName);
    if (openFile == NULL)
    {
        printf("Perf test: unable to open %s\n", FileName);
        return;
    }
    Thread *remove1 = allocThread("remove1");
    remove1->Fork(remove, 0);
    printf("begin writing\n");

    currentThread->Yield();

    numBytes = openFile->Write(Contents, ContentSize);

    delete openFile; // close file
    printf("%s closed file %s\n", currentThread->getName(), FileName);
    Thread *remove2 = allocThread("remove2");
    remove2->Fork(remove, 0);
}

#define PIPE "PipeFile"

void PipeWrite(int which)
{
    OpenFile *pipefile = fileSystem->Open(PIPE);
    char *buffer = new char[SectorSize];
    int size;
    printf("input %s : \n", PIPE);
    getline(&buffer, &size, stdin);
    pipefile->Write(buffer, size);
    delete[] buffer;
    delete pipefile;
}

void PipeRead(int which)
{
    OpenFile *pipefile = fileSystem->Open(PIPE);
    char *buffer = new char[SectorSize];
    pipefile->Read(buffer, pipefile->Length());
    buffer[pipefile->Length()] = '\0';
    printf("output\n:%s\n", buffer);
    delete[] buffer;
    delete pipefile;
}

void PerformanceTest3()
{
    printf("Starting file system performance test:\n");
    if (!fileSystem->Create(PIPE, SectorSize))
    {
        printf("Perf test: can't create %s\n", FileName);
        return;
    }
    Thread *pipewrite = allocThread("pipe write");
    Thread *piperead = allocThread("pipe read");
    pipewrite->Fork(PipeWrite, 0);
    piperead->Fork(PipeRead, 0);
}