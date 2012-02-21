#include "syscall.h"
#include "print.h"

/* Test Acquire Lock*/

int result;
int lock;
int lock1;

int main() {
	
	print("Test Acquire Lock\n");
	
	/*Test1: acquire a normally created lock.*/
	lock = CreateLock("lock", sizeof("lock"));
	lock1 = 100;
	
	print("Test1: create lock %d.\n", lock);
	result = Acquire(lock);
	if (result == 0) {
		print("Test1: try to acquire lock %d that is normally created and succeed.\n", lock);
		Release(lock);
		print("Test1: release lock %d.\n", lock);
	}else {
		print("Test1: try to acquire lock %d that is normally created and fail.\n", lock);
	}
	
	/*Test2: acquire a lock whose index is out of range.*/
	result = Acquire(-1);
	if (result == 0) {
		print("Test2: try to acquire lock %d whose index is out of range and succeed.\n", -1);
		Release(lock);
		print("Test2: release lock %d.\n", -1);
	}else {
		print("Test2: try to acquire lock %d whose index is out of range and fail.\n", -1);
	}
	
	/*Test3: acquire a lock that is not created yet.*/
	lock1 = 100;
	if (result == 0) {
		print("Test3: try to acquire lock %d that is not created yet and succeed.\n", lock1);
		Release(lock1);
		print("Test3: release lock %d.\n", lock1);
	}else {
		print("Test3: try to acquire lock %d that is not created yet and fail.\n", lock1);
	}	
	DestroyLock(lock);
	return 0;
}
