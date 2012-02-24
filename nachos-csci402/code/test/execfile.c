#include "syscall.h"
#include "print.h"

/*
Test Exec_Syscall
*/

int
test_Fork()
{
	print(" ->Successfully invoke Fork().\n");
	Exit(0);
	return 0;
}

int 
test_Exec()
{
	int i = 0;
	print(" ->Successfully invoke Exec().\n");
	for(; i < 3; ++i){ /* we support at most 200 threads in a user program. */
		print(" ->Try to invoke Fork().\n");
		Fork(test_Fork);
	}
	return 0;
}

int main(void)
{
	test_Exec();
	Exit(0);
	return 0;
}
