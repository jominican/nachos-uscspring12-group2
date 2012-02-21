#include "syscall.h"
#include "print.h"

/*Test Destroy Lock*/

int lockID;
int rv;

int main()
{
	
	print("Test Destroy Lock\n");

	lockID = CreateLock("lock1", 5);
	print("Create lock %d.\n", lockID);
	
	Acquire(lockID);
	print("Acquire lock %d.\n", lockID);
	
	Release(lockID);
	print("Release lock %d.\n", lockID);
	
	rv = DestroyLock(lockID);
	if (rv == -1) {
		print("Fail to destroy a lock.\n");
	}else {
		print("DestroyLock lock %d.\n", lockID);
	}
	return 0;
}
