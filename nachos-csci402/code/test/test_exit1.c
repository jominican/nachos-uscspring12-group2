#include "syscall.h"
#include "print.h"

/*
Test system call Exit()
*/

int lockId;
int cvId;

int T1(void)
{
	Acquire(lockId);
	print("T_1 goes to wait.\n");
	Wait(lockId,cvId); /*goes to wait*/
	Release(lockId);
	print("T_1 plans to call Exit.\n");
	Exit(0);
	return 0;
}
int T2(void)
{
	Acquire(lockId);
	print("T_2 goes to wait.\n");
	Wait(lockId,cvId); /*goes to wait*/
	Release(lockId);
	print("T_2 plans to call Exit.\n");
	Exit(0);
	return 0;
}

int T3(void)
{
	Acquire(lockId);
	print("T_3 goes to wait.\n");
	Wait(lockId,cvId);
	Release(lockId);
	print("T_3 plans to call Exit.\n");
	Exit(0);
	return 0;
}

int T4(void)
{
	Acquire(lockId);
	print("T_4 goes to signal.\n");
	Signal(lockId,cvId); /*only one siganl in this user program*/
	Release(lockId);
	print("T_4 plans to call Exit.\n");
	
	Exit(0);
	return 0;
}




int main()
{
	int i = 0;
	lockId = CreateLock("lock", sizeof("lock")); 
	cvId = CreateCondition("CV", sizeof("CV")); 
	
	Fork(T1);
	Fork(T2);
	Fork(T3);
	Fork(T4); /*fork 4 threads*/
	for(; i != 100; ++i)
		Yield();
	print("main plans to call Exit.\n");
	Exit(0);
}