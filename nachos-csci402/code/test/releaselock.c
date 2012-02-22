#include "syscall.h"
#include "print.h"

/*
Test ReleaseLock_Syscall
*/

int result;
int lock;
int lock1;

int main() 
{	
	print("Test Release() system call.\n");
	
	/*Test1: release a normally acquired lock.*/
	lock = CreateLock("lock", sizeof("lock"));
	Acquire(lock);
	print(" ->Test1: acquire lock %d.\n", lock);
	result = Release(lock);
	if (result == 0) {
		print(" ->Test1: try to release lock %d that is normally acquired and succeed.\n", lock);
	}else {
		print(" ->Test1: try to release lock %d that is normally acquired and fail.\n", lock);
	}
	
	/*Test2: release a lock whose index is out of range.*/
	result = Release(-1);
	if (result == 0) {
		print(" ->Test2: try to release lock %d whose index is out of range and succeed.\n", -1);
	}else {
		print(" ->Test2: try to release lock %d whose index is out of range and fail.\n", -1);
	}
	
	/*Test3: release a lock that is not acquired yet.*/
	lock1 = 100;
	if (result == 0) {
		print(" ->Test3: try to release lock %d that is not acquired yet and succeed.\n", lock1);
	}else {
		print(" ->Test3: try to release lock %d that is not acquired yet and fail.\n", lock1);
	}	
	
	DestroyLock(lock);
	/*Test4: fork, release the lock not owned*/
	return 0;
}
