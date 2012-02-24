#include "syscall.h"
#include "print.h"

/*
Test Exec_Syscall
*/

int main(void)
{
	int i = 0;
	print("Test Exec(), Fork(), Exit(), Yield() system calls\n");
	for(; i < 1; ++i){ /* we support at most 3 user programs. */
		print(" ->Try to invoke Exec().\n");
		Exec("../test/doctoroffice", sizeof("../test/doctoroffice"));
	}
	Yield();
	return 0;
}
