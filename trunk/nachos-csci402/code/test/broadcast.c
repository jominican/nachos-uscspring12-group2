#include "syscall.h"
#include "print.h"

int main(void){
	int i = 0;
	Exec("../test/broadcast1", sizeof("../test/broadcast1"));
	for(; i != 100; ++i)
		Yield();
	return 0;
}