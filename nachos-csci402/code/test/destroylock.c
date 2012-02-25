#include "syscall.h"
#include "print.h"

/*
Test DestroyLock_Syscall
*/

int lockID;
int rv;

int main()
{
	print("Test DestroyLock() system call.\n");

	lockID = CreateLock("lock1", 5);
	print(" ->Create lock %d.\n", lockID);
	
	Acquire(lockID);
	print(" ->Acquire lock %d.\n", lockID);
	
	Release(lockID);
	print(" ->Release lock %d.\n", lockID);
	
	print(" ->Test situation destroy an invalid lock id %d.\n", lockID+1);
	DestroyLock(lockID+1);
	print(" ->Test situation destroy a valid lock id %d.\n");
	rv = DestroyLock(lockID);
	if (rv == -1) {
		print(" ->Fail to destroy a lock.\n");
	}else {
		print(" ->DestroyLock lock %d.\n", lockID);
	}
	return 0;
}
