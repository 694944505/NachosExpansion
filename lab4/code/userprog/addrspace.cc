// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "machine.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif
 
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

void int2char(char *name,int a){
    int n=a/10+1;
    
    ASSERT(n<98);
    name[0]='f';
    name[n+1]=0;
    for(int i=0;i<n;i++){
        name[n-i]=a%10+'0';
        a/=10;
    }
}
//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
/*
    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory
*/
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
                    /*
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = machine->allocMemory();
	pageTable[i].valid = TRUE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only

    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }*/
    
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	
	pageTable[i].physicalPage = 0;
	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only

    }
    int2char(fileName,currentThread->getThreadID());
    
    fileSystem->Create(fileName,size);
    diskFile=fileSystem->Open(fileName);
    
    ASSERT(diskFile);
    char *ch;
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        for(int i=0;i<noffH.code.size;i++){
            executable->ReadAt(ch,1, noffH.code.inFileAddr+i);
            diskFile->WriteAt(ch,1, noffH.code.virtualAddr+i);
        }
        
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        for(int i=0;i<noffH.code.size;i++){
            executable->ReadAt(ch,1, noffH.initData.inFileAddr+i);
            diskFile->WriteAt(ch,1, noffH.initData.virtualAddr+i);
        }
    }

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
   fileSystem->Remove(fileName);
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    if(!machine->useInvertedPageTable){
        machine->pageTable = pageTable;
        machine->pageTableSize = numPages;
    }
    
}

void AddrSpace::getPTE(int vpn){
   // printf("111111111111--");
    if(!machine->useInvertedPageTable){
       // printf("111111111111111111111111111");
        pageTable[vpn].lastUseTime=stats->userTicks;
        if(pageTable[vpn].valid) return ;
    /* DEBUG('I',"----------\n");
        for(int i=0;i<numPages;i++){
            if(pageTable[i].valid) DEBUG('I',"%d,",i);
        }
        DEBUG('I',"\n");*/
        int addr=machine->allocMemory();
        int j=0;
        if(addr==-1){
            /*for(int i=0;i<numPages;i++){
                if(pageTable[i].valid&&i!=vpn){
                    addr=pageTable[i].physicalPage;
                    //machine->bitMap->Clear(addr);
                    pageTable[i].valid=FALSE;
                    if(pageTable[i].dirty){
                        diskFile->WriteAt(&(machine->mainMemory[addr*PageSize]),PageSize,i*PageSize);
                    }
                }
            }*/
            
            int minTime=stats->userTicks+1000;

            for(int i=0;i<numPages;i++){
                if(pageTable[i].valid&&i!=vpn){
                    if(pageTable[i].lastUseTime<minTime){
                        minTime=pageTable[i].lastUseTime;
                        j=i;
                    }
                    
                }
            }
            addr=pageTable[j].physicalPage;
            //machine->bitMap->Clear(addr);
            pageTable[j].valid=FALSE;
            if(pageTable[j].dirty){
                diskFile->WriteAt(&(machine->mainMemory[addr*PageSize]),PageSize,j*PageSize);
            }

            
        }

        /*for(int i=0;i<numPages;i++){
            if(pageTable[i].valid) DEBUG('I',"%d,",i);
        }
        DEBUG('I',"%d\n",addr);*/
        ASSERT(addr!=-1);
        DEBUG('I',"vpn %d pagefault, loaded into ppn %d\n", vpn,addr);
        
        diskFile->ReadAt(&(machine->mainMemory[addr*PageSize]),PageSize,vpn*PageSize);
        pageTable[vpn].physicalPage=addr;
        pageTable[vpn].valid=TRUE;
        pageTable[vpn].use=FALSE;
        pageTable[vpn].dirty=FALSE;
        pageTable[vpn].readOnly=FALSE;
        pageTable[vpn].firstUseTime=stats->userTicks;
        return ;
    }else{
       // printf("111111111111");
        for(int i=0;i<NumPhysPages;i++){
            TranslationEntry *t=&machine->invertedPageTable[i];
            if(t->valid&&t->virtualPage==vpn&&t->threadID==currentThread->getThreadID()){
                t->lastUseTime=stats->userTicks;
                return ;
            }
        }
        //TranslationEntry *t=&machine->invertedPageTable[0];
       // printf("%d,%d,%d,%d,\n",t->valid,t->virtualPage,t->threadID,currentThread->getThreadID());
        int pos=-1;
        for(int i=0;i<NumPhysPages;i++){
            if(!machine->invertedPageTable[i].valid){
                pos=i;
                break;
            }
        }
        //printf("112312");
        if(pos==-1){
            int minTime=stats->userTicks+1000;
            
            for(int i=0;i<NumPhysPages;i++){
                TranslationEntry *t=&machine->invertedPageTable[i];
                if(minTime>t->lastUseTime){
                    pos=i;
                    minTime=t->lastUseTime;
                }
            }
            //machine->bitMap->Clear(addr);
            machine->invertedPageTable[pos].valid=FALSE;
            if(pageTable[pos].dirty){
                diskFile->WriteAt(&(machine->mainMemory[pos*PageSize]),PageSize,vpn*PageSize);
            }
        }
       // printf("111111111111");
        DEBUG('I',"vpn %d pagefault, loaded into ppn %d\n", vpn,pos);
       // printf("111111111111");
        diskFile->ReadAt(&(machine->mainMemory[pos*PageSize]),PageSize,vpn*PageSize);
        //printf("111111111111");
        machine->invertedPageTable[pos].virtualPage=vpn;
        machine->invertedPageTable[pos].valid=TRUE;
        machine->invertedPageTable[pos].use=FALSE;
        machine->invertedPageTable[pos].dirty=FALSE;
        machine->invertedPageTable[pos].readOnly=FALSE;
        machine->invertedPageTable[pos].threadID=currentThread->getThreadID();
        machine->invertedPageTable[pos].lastUseTime=stats->userTicks;
        //printf("2222222222222");
        return ;

    }
}