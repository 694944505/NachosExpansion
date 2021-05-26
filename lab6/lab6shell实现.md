# shell实现实习报告



[TOC]

## 内容一：总体概述

本次实验通过修改nachos源码来实现shell。

## 内容二：任务完成情况

### 任务完成列表（Y/N）

|          | Exercise1 |
| -------- | --------- |
| 实习内容 | Y         |

### 具体Exercise的完成情况

#### 实习内容

#####Exercise1 

**设计实现一个用户程序shell，通过./nachos -x shell进入用户交互界面中。在该界面中可以查询支持的功能、可以创建删除文件或目录、可以执行另一个用户程序并输出运行结果，类似Linux上跑的bash。**

在之前实现的文件系统的基础上，在fileSystem类中维护记录当前目录的变量OpenFile *currentDirectoryFile。在fileSystem类中增加以下函数

```

//----------------------------------------------------------------------
// FileSystem::Enter
// 	Enter path "name"
//----------------------------------------------------------------------
bool FileSystem::Enter(char *name)
{
    if(name[0]=='/') return Enter(name, directoryFile);
    else return Enter(name, currentDirectoryFile);
}

bool FileSystem::Enter(char *name, OpenFile *direcFile)
{
    //printf("Enter %s %d\n",name,direcFile->oSector);
    if (name[0] == '/')
        name++;
    int i;
    bool lastLevel = 1;
    for (i = 0; name[i] != 0; i++)
    {
        if (name[i] == '/')
        {
            lastLevel = 0;
            break;
        }
    }
    if (!lastLevel)
        name[i] = 0;
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Entering %s\n", name);
    directory->FetchFrom(direcFile);
    sector = directory->Find(name);

    if (sector >= 0)
        openFile = new OpenFile(sector); // name was found in directory
    delete directory;
    if (lastLevel)
    {
        if(sector>=0){
            currentDirectoryFile=openFile;
            return TRUE;
        }else return FALSE;
    }
    else
    {
        name[i] = '/';
        return Enter(name + i + 1, openFile);
    }
}

//----------------------------------------------------------------------
// FileSystem::Mkdir
// 	make directory "name"
//----------------------------------------------------------------------
bool FileSystem::Mkdir(char *name)
{
    if(name[0]=='/') Mkdir(name, DirectoryFileSize, directoryFile);
    else Mkdir(name, DirectoryFileSize, currentDirectoryFile);
}

bool FileSystem::Mkdir(char *name, int initialSize, OpenFile *direcFile)
{
    //printf("--Making %s %d\n",name,direcFile->oSector);
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    directory = new Directory(NumDirEntries);
    directory->FetchFrom(direcFile);

    //if(direcFile!=directoryFile) delete direcFile;
    if (name[0] == '/')
        name++;
    int i;
    bool lastLevel = 1;
    for (i = 0; name[i] != 0; i++)
    {
        if (name[i] == '/')
        {
            lastLevel = 0;
            break;
        }
    }

    if (name[i] != 0)
        name[i] = 0;
    DEBUG('f', "Creating directory %s, size %d\n", name, initialSize);
    sector = directory->Find(name);
    if (sector != -1)
    {
        if (lastLevel)
            return FALSE; // file is already in directory
        else
        {
            name[i] = '/';
            return Create(name + i + 1, initialSize, new OpenFile(sector));
            ;
        }
    }
    else
    {

        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find(); // find a sector to hold the file header

        if (sector == -1)
            success = FALSE; // no free block for file header
        else if (!directory->Add(name, sector, !lastLevel))
            success = FALSE; // no space in directory
        else
        {

            hdr = new FileHeader;
            if (lastLevel)
            {
                if (!hdr->Allocate(freeMap, initialSize))
                    return FALSE; // no space on disk for data
            }
            else
            {
                if (!hdr->Allocate(freeMap, DirectoryFileSize))
                    return FALSE; // no space on disk for data
            }

            {

                hdr->setCreateTime();
                hdr->setLastModifiedTime();
                hdr->setLastVisitTime();
                hdr->setType(name);
                // everthing worked, flush all changes back to disk
                hdr->WriteBack(sector);
                directory->WriteBack(direcFile);
                freeMap->WriteBack(freeMapFile);

                if (lastLevel)
                {
                    success = TRUE;
                    OpenFile *opp = new OpenFile(sector);
                    Directory *mydirectory = new Directory(NumDirEntries,sector,direcFile->oSector);
                    mydirectory->WriteBack(opp);
                    name[i] = '/';
                }
                else
                {
                    OpenFile *opp = new OpenFile(sector);
                    Directory *mydirectory = new Directory(NumDirEntries,sector,direcFile->oSector);
                    mydirectory->WriteBack(opp);

                    success = Create(name + i + 1, initialSize, opp);
                    name[i] = '/';
                }
            }
            delete hdr;
        }
        //freeMap->Print();
        delete freeMap;
    }
    delete directory;

    return success;
}
```

修改main函数，将-x选项解析到/usreprog/progtest文件的SynchConsoleTest方法。在SynchConsoleTest中实现了退出、查看帮助、查看目录文件、切换目录、新建目录、删除目录或文件、新建文件、查看文件内容、复制UNIX文件到Nachos文件系统、执行用户程序功能，如下：

```
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
            //printf("%d %d %c\n",i,j,ch);
            ch = synchConsole->GetChar();
            //synchConsole->PutChar(ch);
            if(ch=='\n'){
                //printf("shell:");
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
```

测试文件复制和用户执行程序如下：

![1](C:\Users\69494\Desktop\软微\操统\lab6\images\1.PNG)

测试文件查看如下：

![2](C:\Users\69494\Desktop\软微\操统\lab6\images\2.PNG)

测试目录跳转如下：

![3](C:\Users\69494\Desktop\软微\操统\lab6\images\3.PNG)

## 内容三：遇到的困难以及解决方法

1. 连续操作一段指令之后经常会有bug。

   > 分开测试了不同的功能，实际上没有解决完bug。


## 内容四：收获及感想

这次实验的综合性是最强的，对nachos的代码又整体回顾了一遍。

## 内容五：对课程的意见和建议

无

## 内容六：参考文献

2. [Nachos中文教程 - 百度文库 (baidu.com)](https://wenku.baidu.com/view/47ba36d4d1f34693daef3ed7.html)
