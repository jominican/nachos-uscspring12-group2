#include "syscall.h"
#include "print.h"
/*
Test System Call: Signal, Wait, BroadCast.
*/

int lockid_1;
int lockid_2;
int cvid_1;
int cvid_2;

int T_1(){
	print("T_1 tries to acquire lock %d.\n", lockid_1);
	Acquire(lockid_1);
	print("T_1 owns lock %d.\n", lockid_1);
	/*
	T_1 do something in the critical section.
	*/
	print("Test situation the thread wait on CV id %d with a lock id %d which it doesn't own.\n", cvid_1, lockid_2);
	Wait(lockid_2,cvid_1);
	print("T_1 plans to wait on CV %d.\n", cvid_1);
	Wait(lockid_1, cvid_1);
	print("T_1 was signaled on CV %d just now.\n", cvid_1);
	
	print("T_1 tries to signal T_2 on CV %d.\n", cvid_2);
	Signal(lockid_1,cvid_2);
	
	print("T_1 tries to release lock %d.\n", lockid_1);
	Release(lockid_1);
	Exit(0);
	return 0;
	
}

int T_2(){
	print("T_2 tries to acquire lock %d.\n", lockid_1);
	Acquire(lockid_1);
	print("T_2 owns lock %d.\n", lockid_1);
	/*
	T_2 do something in the critical section.
	*/
	print("Test situation the thread signal on a cv id %d with a lock id %d which it doesn't own.\n", cvid_1,lockid_2);
	Signal(lockid_2,cvid_1);
	
	print("T_2 tries to signal T_1 on CV %d.\n", cvid_1);
	Signal(lockid_1, cvid_1);
	
	print("T_2 tries to wait on CV %d.\n", cvid_2);
	Wait(lockid_1, cvid_2);
	print("T_2 was signaled on CV %d just now.\n", cvid_2);
	
	print("T_2 tries to release lock %d.\n", lockid_1);
	Release(lockid_1);
	Exit(0);
	return 0;
	
}


int main(void)
{
	int i = 0;
	lockid_1 = CreateLock("lock1", sizeof("lock1"));
	lockid_2 = CreateLock("lock2", sizeof("lock2"));
	cvid_1 = CreateCondition("CV_1", sizeof("CV_1"));
	cvid_2 = CreateCondition("CV_2", sizeof("CV_2"));
	print("Successfully created one lock and two condition variables.\n");
	
	print("Fork T_1.\n");
	Fork(T_1);
	print("Fork T_2.\n");
	Fork(T_2);
	for(; i < 100; ++i)
		Yield();
	Exit(0);
	return 0;
	
}
