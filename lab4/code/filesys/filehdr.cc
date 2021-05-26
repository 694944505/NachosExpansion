// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include <time.h>

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    DEBUG('f', "Allocate size %d\n", fileSize);
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    int numLeft=freeMap->NumClear();
    //printf("%d %d %d %d\n",fileSize,numSectors,MaxFileSize,NumOfFirstIndex);
    if(numSectors<=NumDirect){
        if (numLeft < numSectors)
        return FALSE;		// not enough space
        for (int i = 0; i < numSectors; i++)
            dataSectors[i] = freeMap->Find();
        return TRUE;
    }
    if(numSectors<=NumDirect+NumOfFirstIndex){
        if(numLeft < numSectors+1)
            return FALSE;		// not enough space
        for (int i = 0; i < NumDirect; i++)
            dataSectors[i] = freeMap->Find();
        dataSectors[NumDirect]=freeMap->Find();
        int firstIndex[NumOfFirstIndex];
        for(int i=0;i<numSectors-NumDirect;i++){
            firstIndex[i]=freeMap->Find();
        }
        synchDisk->WriteSector(dataSectors[NumDirect],(char *)firstIndex);
        return TRUE;
    }
    if(numSectors<=NumDirect+NumOfFirstIndex+NumOfSecondIndex){
        int secondSectorNum=numSectors-NumDirect-NumOfFirstIndex;
        int secondIndexNum=(secondSectorNum%NumOfFirstIndex==0?
            secondSectorNum/NumOfFirstIndex:secondSectorNum/NumOfFirstIndex+1);
        //printf("---%d %d",secondSectorNum,secondIndexNum);
        if(numLeft < numSectors+2+secondIndexNum)
            return FALSE;		// not enough space
        for(int i=0;i<=NumDirect;i++){
            dataSectors[i] = freeMap->Find();
            
        }
        dataSectors[NumDirect]=freeMap->Find();
        int firstIndex[NumOfFirstIndex],secondIndex[NumOfFirstIndex];
        for(int i=0;i<NumOfFirstIndex;i++){
            firstIndex[i]=freeMap->Find();
            
        }
        synchDisk->WriteSector(dataSectors[NumDirect],(char *)firstIndex);
        dataSectors[NumDirect+1]=freeMap->Find();
        for(int i=0;i<secondIndexNum;i++){
            firstIndex[i]=freeMap->Find();
            for(int j=0;j<NumOfFirstIndex&&j+i*NumOfFirstIndex<secondSectorNum;j++){
                secondIndex[j]=freeMap->Find();
            }
            synchDisk->WriteSector(firstIndex[i],(char *)secondIndex);
        }
        synchDisk->WriteSector(dataSectors[NumDirect+1],(char *)firstIndex);
        return TRUE;
    }
    ASSERT(FALSE);
}

//----------------------------------------------------------------------
// FileHeader::Extend
// 	Extend file size 
//	"freeMap" is the bit map of free disk sectors
//	"num" is the bits added
//----------------------------------------------------------------------

bool
FileHeader::Extend(int num){
    DEBUG('f', "Extend size %d %d\n",numBytes, num);
    BitMap *freeMap=new BitMap(NumSectors);
    fileSystem->GetBitMap(freeMap);
    numBytes+=num;
    if(divRoundUp(numBytes, SectorSize)!=numSectors){
        
        Deallocate(freeMap);
        
        numSectors=divRoundUp(numBytes, SectorSize);
        
        bool is=Allocate(freeMap,numBytes);
        fileSystem->SetBitMap(freeMap);
        return is;
    }
   
    return TRUE;
    
}
//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    DEBUG('f', "Deallocate size %d\n",numSectors);
    if(numSectors<=NumDirect){
        for (int i = 0; i < numSectors; i++){
            //printf("-------%d\n",dataSectors[i]);
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[i]);
        }
        return ;
    }
    if(numSectors<=NumDirect+NumOfFirstIndex){
        for (int i = 0; i < NumDirect+1; i++){
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[i]);
        }
        int firstIndex[NumOfFirstIndex];
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        for(int i=0;i<numSectors-NumDirect;i++){
            ASSERT(freeMap->Test((int) firstIndex[i]));  // ought to be marked!
	        freeMap->Clear((int) firstIndex[i]);
        }
        return ;
    }
    if(numSectors<=NumDirect+NumOfFirstIndex+NumOfSecondIndex){
        int secondSectorNum=numSectors-NumDirect-NumOfFirstIndex;
        int secondIndexNum=(secondSectorNum%NumOfFirstIndex==0?
            secondSectorNum/NumOfFirstIndex:secondSectorNum/NumOfFirstIndex+1);
        
        for(int i=0;i<=NumDirect+2;i++){
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	        freeMap->Clear((int) dataSectors[i]);
        }
        
        int firstIndex[NumOfFirstIndex],secondIndex[NumOfFirstIndex];
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        for(int i=0;i<NumOfFirstIndex;i++){
            ASSERT(freeMap->Test((int) firstIndex[i]));  // ought to be marked!
	        freeMap->Clear((int) firstIndex[i]);
        }
        synchDisk->ReadSector(dataSectors[NumDirect+1],(char *)firstIndex);
        for(int i=0;i<secondIndexNum;i++){
            synchDisk->ReadSector(firstIndex[i],(char *)secondIndex);
            for(int j=0;j<NumOfFirstIndex&&j+i*NumOfFirstIndex<secondSectorNum;j++){
                ASSERT(freeMap->Test((int) secondIndex[j]));  // ought to be marked!
	            freeMap->Clear((int) secondIndex[j]);
            }
            ASSERT(freeMap->Test((int) firstIndex[i]));  // ought to be marked!
	        freeMap->Clear((int) firstIndex[i]);
        }
        
        return ;
    }
    ASSERT(FALSE);
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int numSectors=offset/SectorSize;
    if(numSectors<NumDirect){
        return dataSectors[numSectors];
    }
    if(numSectors<NumDirect+NumOfFirstIndex){
        int firstIndex[NumOfFirstIndex];
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        return firstIndex[numSectors-NumDirect];
    }
    if(numSectors<NumDirect+NumOfFirstIndex+NumOfSecondIndex){
        int secondSectorNum=numSectors-NumDirect-NumOfFirstIndex;
        int secondIndexNum=secondSectorNum/NumOfFirstIndex;
        int firstIndex[NumOfFirstIndex],secondIndex[NumOfFirstIndex];
        synchDisk->ReadSector(dataSectors[NumDirect+1],(char *)firstIndex);
        synchDisk->ReadSector(firstIndex[secondIndexNum],(char *)secondIndex);
        return secondIndex[secondSectorNum%NumOfFirstIndex];
    }
    ASSERT(FALSE);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

void printChar(char ch){
    if ('\040' <= ch && ch <= '\176')   // isprint(ch)
		printf("%c", ch);
    else
		printf("\\%x", (unsigned char)ch);
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i,j,k;
    char *data = new char[SectorSize];
    printf("---------------****---------------\n");
    printf("FileHeader contents.  File size: %d.  File blocks used:\n", numBytes);
    if(numSectors<=NumDirect){
        printf("Direct blocks:");
        for (int i = 0; i < numSectors; i++)
            printf("%d ",dataSectors[i]);
        printf("\n");
    }
    else if(numSectors<=NumDirect+NumOfFirstIndex){
        printf("Direct blocks:");
        for (int i = 0; i < NumDirect; i++)
            printf("%d ",dataSectors[i]);
        printf("\n");
        int firstIndex[NumOfFirstIndex];
        printf("\n1-level indexs in sector %d:",dataSectors[NumDirect]);
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        for(int i=0;i<numSectors-NumDirect;i++){
            printf("%d ",firstIndex[i]);
        }
        printf("\n");
    }
    else if(numSectors<=NumDirect+NumOfFirstIndex+NumOfSecondIndex){
        printf("Direct blocks:");
        for (int i = 0; i < NumDirect; i++)
            printf("%d ",dataSectors[i]);
        int firstIndex[NumOfFirstIndex],secondIndex[NumOfFirstIndex];
        printf("\nblocks in level-1 index %d:",dataSectors[NumDirect]);
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        for(int i=0;i<NumOfFirstIndex;i++){
            printf("%d ",firstIndex[i]);
        }
        
        int secondSectorNum=numSectors-NumDirect-NumOfFirstIndex;
        int secondIndexNum=(secondSectorNum%NumOfFirstIndex==0?
            secondSectorNum/NumOfFirstIndex:secondSectorNum/NumOfFirstIndex+1);

        synchDisk->ReadSector(dataSectors[NumDirect+1],(char *)firstIndex);
        printf("\nlevel-1 indexs in level-2 index %d:\n",dataSectors[NumDirect+1]);
        for(int i=0;i<secondIndexNum;i++){
            synchDisk->ReadSector(firstIndex[i],(char *)secondIndex);
            printf("\nblocks in level-1 index %d:",firstIndex[i]);
            for(int j=0;j<NumOfFirstIndex&&j+i*NumOfFirstIndex<secondSectorNum;j++){
                printf("%d ",secondIndex[j]);
            }
            printf("\n");
        }
    }

    printf("\nFile type: %s\n", type);
    printf("Create time: %s", createTime);
    printf("Last visit time: %s", lastVisitTime);
    printf("Last modified time: %s", lastModifiedTime);
    

    printf("\nFile contents:\n");

    if(numSectors<=NumDirect){
        for ( i = 0,k=0; i < numSectors; i++){
            synchDisk->ReadSector(dataSectors[i], data);
            for( j=0;j<SectorSize&&k<numBytes;j++,k++){
                printChar(data[j]);
            }
            printf("\n");
        }   
    }
    else if(numSectors<=NumDirect+NumOfFirstIndex){
        for ( i = 0,k=0; i < NumDirect; i++){
            synchDisk->ReadSector(dataSectors[i], data);
            for( j=0;j<SectorSize&&k<numBytes;j++,k++){
                printChar(data[j]);
            }
            printf("\n");
        } 
        int firstIndex[NumOfFirstIndex];
        
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        for(i=0;i<numSectors-NumDirect;i++){
            synchDisk->ReadSector(firstIndex[i], data);
            for( j=0;j<SectorSize&&k<numBytes;j++,k++){
                printChar(data[j]);
            }
            printf("\n");
        }
        
    }
    else if(numSectors<=NumDirect+NumOfFirstIndex+NumOfSecondIndex){
        for ( i = 0,k=0; i < NumDirect; i++){
            synchDisk->ReadSector(dataSectors[i], data);
            for( j=0;j<SectorSize&&k<numBytes;j++,k++){
                printChar(data[j]);
            }
            printf("\n");
        } 
        int firstIndex[NumOfFirstIndex],secondIndex[NumOfFirstIndex];
        
        synchDisk->ReadSector(dataSectors[NumDirect],(char *)firstIndex);
        for(i=0;i<NumOfFirstIndex;i++){
            synchDisk->ReadSector(firstIndex[i], data);
            for( j=0;j<SectorSize&&k<numBytes;j++,k++){
                printChar(data[j]);
            }
            printf("\n");
        }
        
        int secondSectorNum=numSectors-NumDirect-NumOfFirstIndex;
        int secondIndexNum=(secondSectorNum%NumOfFirstIndex==0?
            secondSectorNum/NumOfFirstIndex:secondSectorNum/NumOfFirstIndex+1);

        synchDisk->ReadSector(dataSectors[NumDirect+1],(char *)firstIndex);
        for(i=0;i<secondIndexNum;i++){
            synchDisk->ReadSector(firstIndex[i],(char *)secondIndex);
            for(j=0;j<NumOfFirstIndex&&j+i*NumOfFirstIndex<secondSectorNum;j++){
                synchDisk->ReadSector(secondIndex[j], data);
                for(int x=0;x<SectorSize&&k<numBytes;x++,k++){
                    printChar(data[x]);
                }
            printf("\n");
            } 
        }
    }
    delete [] data;
}


char *getTime(){
    time_t t;
    time(&t);
    return asctime(gmtime(&t));
}
char *getType(char *name)//获取文件后缀
{
    char *dot = strrchr(name, '.');
    if(!dot || dot == name) return "";
    return dot + 1;
}

