#include "syscall.h"
#include "print.h"

/*
Test Exec_Syscall
*/

int
test_Fork(){
	print("Successfully Created Thread.\n"); /*indicate that this thread is running*/
	Exit(0);
	return 0;
}

int 
test_Exec(){
	int i = 0;
	print("Successfully Created user program.\n");
	for(; i < 3; ++i){  /*Fork 3 threads*/
		print("Try to invoke Fork().\n");
		Fork(test_Fork);
	}
	return 0;
}

int main(){
	test_Exec();
	/*Yield();*/
	Exit(0);
	return 0;
}