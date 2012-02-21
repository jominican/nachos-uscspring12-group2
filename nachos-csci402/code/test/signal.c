#include "syscall.h"
#include "print.h"
/*
 Test Signal
 
 */

int lockId;
int cvId;
int result;

void T1(){
	
	Acquire(lockId);
	print("Thread 1 acquire lock %d.\n", lockId);
	
	Wait(lockId, cvId);
	print("Thread 1 wait on Condition %d with Lock %d.\n", cvId, lockId);
	
	Release(lockId);
	print("Thread 1 release lock %d.\n", lockId);
}

void T2(){
	
	Acquire(lockId);
	print("Thread 2 acquire lock %d.\n", lockId);
	
	result = Signal(lockId, cvId);
	if (result == -1) {
		print("Lock %d is not validated. Thread 2 fail to signal Thread 1.\n", lockId);
		Release(lockId);
		print("Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else if (result == -2){
		print("Condition %d is not validated. Thread 2 fail to signal Thread 1.\n", cvId);
		Release(lockId);
		print("Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else {
		print("Thread 2 succeed in signaling Thread 1 on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print("Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}
}

int main(){
	
	print("Test Signal\n");
	
	lockId = CreateLock("abc",3);
	cvId = CreateCondition("abc",3);
	
	T1();
	T2();
	return 0;
}

