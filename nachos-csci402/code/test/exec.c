#include "syscall.h"
#include "print.h"
/*
Test Exec_Syscall
*/

int main(){
	/*
	print("Try to invoke Exec().\n");
	Exec("../test/test_ExecFile", sizeof("../test/test_ExecFile"));
	*/
	int i = 0, j = 0;
	for(; i < 3; ++i){
		print("Try to invoke Exec().\n");
		Exec("../test/test_ExecFile", sizeof("../test/test_ExecFile"));
	}
	Yield();
	/*Exit(0);*/
	/*
	for(; j < 100; ++j)
		Yield();
	*/
	return 0;
}