#include "syscall.h"
#include "print.h"

/*
The user program we would execute.
*/

int getInput(void)
{
	char buf[1];
	
	Read(buf, 1, ConsoleInput);
	if(buf[0] < '0' || buf[0] > '9')	return -1;
	else buf[0] - '0';
}

int main(void)
{
	int i = 0;
	int num = 0;
	
	print("Please input the number of the doctor office.\n");
	num = getInput();
	if(num < 0 || num > 3){
		print("The interval of the doctor office user program should be [1, 3].\n");
		return 0;
	}
	
	for(i = 0; i < num; ++i){ /* we support at most 3 user programs. */
		Exec("../test/doctorofficefile", sizeof("../test/doctorofficefile"));
	}
	Yield();
	return 0;
}
