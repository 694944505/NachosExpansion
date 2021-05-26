// filehdr.h
//	Data structures for managing a disk file header.
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"
#include <string.h>

#define LenOfType 5
#define LenOfTime 26
#define LenOfString (LenOfType + 3 * LenOfTime)
#define NumFileSector ((SectorSize - (2 * sizeof(int) + LenOfString * sizeof(char))) / sizeof(int))
#define NumDirect (NumFileSector - 2)
#define NumOfFirstIndex (SectorSize / sizeof(int))
#define NumOfSecondIndex (NumOfFirstIndex * NumOfFirstIndex)
#define MaxFileSize ((NumDirect + NumOfFirstIndex + NumOfSecondIndex) * SectorSize)

//#define NumDirect 	((SectorSize - 2 * sizeof(int)) / sizeof(int))

#define MaxFileSizeOld (NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks.
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

char *getTime();
char *getType(char *name);
class FileHeader
{
public:
  bool Allocate(BitMap *bitMap, int fileSize); // Initialize a file header,
      //  including allocating space
      //  on disk for the file data
  void Deallocate(BitMap *bitMap); // De-allocate this file's
      //  data blocks

  void FetchFrom(int sectorNumber); // Initialize file header from disk
  void WriteBack(int sectorNumber); // Write modifications to file header
                                    //  back to disk

  int ByteToSector(int offset); // Convert a byte offset into the file
                                // to the disk sector containing
                                // the byte

  int FileLength(); // Return the length of the file
                    // in bytes

  void Print(); // Print the contents of the file.
  void setType(char *name) { strcpy(type, getType(name)); }
  void setCreateTime() { strcpy(createTime, getTime()); }
  void setLastVisitTime() { strcpy(lastVisitTime, getTime()); }
  void setLastModifiedTime() { strcpy(lastModifiedTime, getTime()); }
  bool Extend(int num);

private:
  int numBytes;                   // Number of bytes in the file
  int numSectors;                 // Number of data sectors in the file
  int dataSectors[NumFileSector]; // Disk sector numbers for each data
                                  // block in the file
  char createTime[LenOfTime];
  char lastVisitTime[LenOfTime];
  char lastModifiedTime[LenOfTime];
  char type[LenOfType];
};

#endif // FILEHDR_H
