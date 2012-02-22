#include "syscall.h"
#include "print.h"

/* 
Test CreateLock_Syscall
*/

int lockID;
int rv;

int main()
{	
	print("Test CreateLock() system call.\n");
	
	lockID = CreateLock("lock1", 5);
	if (lockID == -1) {
		print(" ->Fail to create a lock.\n");
	}else {
		print(" ->Create lock %d.\n", lockID);
		
		Acquire(lockID);
		print(" ->Acquire lock %d.\n", lockID);
		
		Release(lockID);
		print(" ->Release lock %d.\n", lockID);
		
		rv = DestroyLock(lockID);
		print(" ->DestroyLock lock %d.\n", lockID);
	}
	return 0;
}
