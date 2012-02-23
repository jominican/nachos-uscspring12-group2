#include "syscall.h"
#include "print.h"

/*
Test BroadCast_Syscall
*/

int lockId;
int cvId;
int result;

void T1(){
	
	Acquire(lockId);
	print(" ->Thread 1 acquire lock %d.\n", lockId);
	
	Wait(lockId, cvId);
	print(" ->Thread 1 wait on Condition %d with Lock %d.\n", cvId, lockId);
	
	Release(lockId);
	print(" ->Thread 1 release lock %d.\n", lockId);
	Exit(0);
}

void T3(){
	
	Acquire(lockId);
	print(" ->Thread 3 acquire lock %d.\n", lockId);
	
	Wait(lockId, cvId);
	print(" ->Thread 3 wait on Condition %d with Lock %d.\n", cvId, lockId);
	
	Release(lockId);
	print(" ->Thread 3 release lock %d.\n", lockId);
	Exit(0);
}

void T2(){
	
	Acquire(lockId);
	print(" ->Thread 2 acquire lock %d.\n", lockId);
	
	result = BroadCast(lockId, cvId);
	if (result == -1) {
		print(" ->Lock %d is not validated. Thread 2 fail to broadcast Thread 1.\n", lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else if (result == -2){
		print(" ->Condition %d is not validated. Thread 2 fail to broadcast Thread 1.\n", cvId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else {
		print(" ->Thread 2 succeed in broadcasting Thread 1 on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}
	Exit(0);
}

int main()
{	
	print("Test Broadcast() system call.\n");	
	
	lockId = CreateLock("abc",3);
	cvId = CreateCondition("abc",3);
	
	Fork(T1);
	Fork(T3);
	Fork(T2);
	Exit(0);
	return 0;
}
