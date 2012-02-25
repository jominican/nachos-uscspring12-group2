#include "syscall.h"
#include "print.h"

/*
Test Wait_Syscall
*/

int lockId;
int cvId;
int result;

void T1()
{	
	int invalidLockId = lockId + 1;
	int invalidCVId = cvId +1;
	Acquire(lockId);
	print(" ->Thread 1 acquire lock %d.\n", lockId);
	print(" ->Test situation the thread wait on a invalid lock id %d.\n", invalidLockId);
	Wait(invalidLockId, invalidCVId);
	print(" ->Test situation the thread wait on a invalid CV id %d.\n", invalidCVId);
	Wait(lockId, invalidCVId);
	
	print(" ->Test situation the thread wait on a valid lock id %d and CV id %d.\n", lockId, cvId);
	result = Wait(lockId, cvId);
	if (result == -1) {
		print(" ->Lock %d is not validated. Thread 1 fail to wait.\n", lockId);
		Release(lockId);
		print(" ->Thread 1 release lock %d.\n", lockId);
	}else if (result == -2){
		print(" ->Condition %d is not validated. Thread 1 fail to wait.\n", cvId);
		Release(lockId);
		print(" ->Thread 1 release lock %d.\n", lockId);
	}else {
		print(" ->Thread 1 was signaled on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 1 release lock %d.\n", lockId);
	}
	Exit(0);
}

void T2(){
	
	Acquire(lockId);
	print(" ->Thread 2 acquire lock %d.\n", lockId);
	result = Signal(lockId, cvId);
	if (result == -1) {
		print(" ->Thread 1 fail in waiting on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
	}else if (result == -2){
		print(" ->Thread 1 fail in waiting on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
	}else {
		Signal(lockId, cvId);
		print(" ->Thread 2 signal Thread 1 on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
	}
	Exit(0);
}

int main()
{	
	int i = 0;
	print("Test Wait() system call.\n");	
	
	lockId = CreateLock("abc",3);
	cvId = CreateCondition("abc",3);
	
    Fork(T1);
	Fork(T2);
	for(; i != 100; ++i){
		Yield();
	}
	Exit(0);
	return 0;
}
