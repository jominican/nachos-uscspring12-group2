#include "syscall.h"
#include "print.h"

/*
Test Signal_Syscall
*/

int main()
{	
	int i = 0;
	print("Test Signal() system call.\n");	
	Exec("../test/signal1", sizeof("../test/signal1"));
	for(; i != 100; ++i)
		Yield();
	return 0;
}
