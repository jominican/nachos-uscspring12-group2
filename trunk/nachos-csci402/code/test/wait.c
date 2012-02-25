#include "syscall.h"
#include "print.h"

/*
Test Wait_Syscall
*/


int main()
{	
	int i = 0;
	print("Test Wait() system call.\n");	
	Exec("../test/wait1", sizeof("../test/wait1"));
	for(; i != 100; ++i);
		Yield();
	
	return 0;
}
