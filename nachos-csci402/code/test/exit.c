#include "syscall.h"
#include "print.h"
/*
Test system call Exit()
*/

int processId = 0;

int main(void)
{
	int i = 0;
	print("Test Exit system call.\n");
	for(; i != 2; ++i)
		Exec("../test/test_exit1",sizeof("../test/test_exit1"));
	for(; i !=100; ++i)
		Yield();
}