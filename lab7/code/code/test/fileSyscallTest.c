#include "syscall.h"

#define Len 44
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
	Write("Read result:",12,ConsoleOutput);
	Write(buffer,Len,ConsoleOutput);
	Exit(0);
}
