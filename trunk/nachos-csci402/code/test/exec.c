#include "syscall.h"
#include "print.h"
/*
Test Exec_Syscall
*/

int main(){
	int i = 0, j = 0;
	for(; i < 3; ++i){
		print("Try to invoke Exec().\n");
		Exec("../test/test_ExecFile", sizeof("../test/test_ExecFile"));
	}
	/*
	for(; j < 100; ++j)
		Yield();
	*/
	return 0;
}