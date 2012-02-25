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
	
	print(" ->Thread 1 wait on Condition %d with Lock %d.\n", cvId, lockId);
	Wait(lockId, cvId);
	print(" ->Thread 1 was signaled just now on %d with Lock %d.\n", cvId, lockId);
	
	
	Release(lockId);
	print(" ->Thread 1 release lock %d.\n", lockId);
	Exit(0);
}

void T2(){
	
	Acquire(lockId);
	print(" ->Thread 2 acquire lock %d.\n", lockId);
	
	print(" ->Thread 2 wait on Condition %d with Lock %d.\n", cvId, lockId);
	Wait(lockId, cvId);
	print(" ->Thread 2 was signaled just now on %d with Lock %d.\n", cvId, lockId);
	
	
	Release(lockId);
	print(" ->Thread 2 release lock %d.\n", lockId);
	Exit(0);
}


void T3(){
	
	Acquire(lockId);
	print(" ->Thread 3 acquire lock %d.\n", lockId);
	
	print(" ->Test situation that the thread broadcast an invalid lock id %d.\n", lockId+1);
	BroadCast(lockId+1, cvId);
	print(" ->Test situation that the thread broadcast an invalid CV id %d.\n", cvId+1);
	BroadCast(lockId, cvId);
	
	print("v->Test situation that that the thread broadcast on a valid cv id %d and valid lock id %d. \n", cvId, lockId);
	result = BroadCast(lockId, cvId);
	if (result == -1) {
		print(" ->Lock %d is not validated. Thread 3 fail to broadcast the threads in the waiting queue.\n", lockId);
		Release(lockId);
		print(" ->Thread 3 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else if (result == -2){
		print(" ->Condition %d is not validated. Thread 3 fail to broadcast threads in the waiting queue.\n", cvId);
		Release(lockId);
		print(" ->Thread 3 release lock %d.\n", lockId);
		DestroyCondition(cvId);
		DestroyLock(lockId);
	}else {
		print(" ->Thread 3 succeed in broadcasting all the threads on Condition %d with Lock %d.\n", cvId, lockId);
		Release(lockId);
		print(" ->Thread 3 release lock %d.\n", lockId);
		/*DestroyCondition(cvId);
		DestroyLock(lockId);*/
	}
	Exit(0);
}

int main()
{	
	int i = 4;
	print("Test Broadcast() system call.\n");	
	
	lockId = CreateLock("abc",3);
	cvId = CreateCondition("abc",3);
	
	Fork(T1);
	Fork(T2);
	Fork(T3);
	Exit(0);
	return 0;
}
