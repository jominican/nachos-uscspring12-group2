#include "syscall.h"
#include "print.h"
/*
Test System Call: Signal, Wait.
*/



int main(void)
{
	int i = 0;
	Exec("../test/testcv1", sizeof("../test/testcv1")); /* executue the test file*/
	for(; i < 100; ++i)
		Yield();
}
