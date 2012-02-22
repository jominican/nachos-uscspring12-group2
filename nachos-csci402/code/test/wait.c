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
	Acquire(lockId);
	print(" ->Thread 1 acquire lock %d.\n", lockId);
	
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
		print(" ->Thread 1 succeed in waiting on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 1 release lock %d.\n", lockId);
	}
}

void T2(){
	
	Acquire(lockId);
	print(" ->Thread 2 acquire lock %d.\n", lockId);
	
	if (result == -1) {
		print(" ->Thread 1 fail in waiting on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else if (result == -2){
		print(" ->Thread 1 fail in waiting on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else {
		Signal(lockId, cvId);
		print(" ->Thread 2 signal Thread 1 on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}
}

int main()
{	
	print("Test Wait() system call.\n");	
	
	lockId = CreateLock("abc",3);
	cvId = CreateCondition("abc",3);
	
	T1();
	T2();
	return 0;
}
