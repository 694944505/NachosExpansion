#include "syscall.h"

int f(){
	Write("fork thread function finished",30,ConsoleOutput);
	Exit(0);
}

int main(){
	Write("main thread function start",27,ConsoleOutput);
	Fork(f);
	Join(Exec("./test/fileTest"));
	Write("main thread function finished",30,ConsoleOutput);
	Yield();
	Exit(0);
}