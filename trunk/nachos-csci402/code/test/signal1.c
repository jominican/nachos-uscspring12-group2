#include "syscall.h"
#include "print.h"

/*
Test Signal_Syscall
*/

int lockId;
int cvId;
int result;

void T1(){
	
	Acquire(lockId);
	print(" ->Thread 1 acquire lock %d.\n", lockId);
	
	print(" ->Thread 1 wait on Condition %d with Lock %d.\n", cvId, lockId);
	Wait(lockId, cvId);
	
	Release(lockId);
	print(" ->Thread 1 release lock %d.\n", lockId);
	Exit(0);
}

void T2(){
	int invalidLockId = lockId+1;
	int invalidCVId = cvId+1;
	Acquire(lockId);
	print(" ->Thread 2 acquire lock %d.\n", lockId);
	
	print(" ->Test the situation the thread use a invalid lock id %d.\n", invalidLockId);
	result = Signal(invalidLockId, cvId);
	
	print(" ->Test the situation the thread use a invalid CV id %d.\n", invalidCVId);
	result = Signal(lockId, invalidCVId);
	
	print(" ->Test the situation the thread use a valid lock id and valid CV id.\n");
	result = Signal(lockId, cvId);
	if (result == -1) {
		print(" ->Lock %d is not validated. Thread 2 fail to signal Thread 1.\n", lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else if (result == -2){
		print(" ->Condition %d is not validated. Thread 2 fail to signal Thread 1.\n", cvId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else {
		print(" ->Thread 2 succeed in signaling Thread 1 on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 2 release lock %d.\n", lockId);
	}
	Exit(0);
}

int main()
{	
	int i = 0;
	print("Test Signal() system call.\n");	
	
	lockId = CreateLock("abc",3);
	cvId = CreateCondition("abc",3);
    
	Fork(T1);
	Fork(T2);
	for(; i != 100; ++i)
		Yield();
	Exit(0);
	return 0;
}
