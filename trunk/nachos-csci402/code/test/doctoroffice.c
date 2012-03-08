#include "syscall.h"
#include "print.h"

/*
The user program we would execute.
*/


int main(void)
{
	int i = 0;
	int num = 0;
	
	print("Please input the number of the doctor office.\n");
	num = Scanf();
	if(num < 0 || num > 2){
		print("The interval of the doctor office user program should be [1, 2].\n");
		return 0;
	}
	
	for(i = 0; i < num; ++i){ /* we support at most 2 user programs. */
		Exec("../test/doctorofficefile", sizeof("../test/doctorofficefile"));
	}
	Yield();
	return 0;
}
