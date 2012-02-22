#include "syscall.h"
#include "print.h"

/*
Test DestroyCondition_Syscall
*/

int cvID;
int cvRV;

int main()
{	
	print("Test DestroyCondition() system call.\n");
	
	cvID = CreateCondition("cv1", 3);
	print(" ->Create cv %d.\n", cvID);
	
	cvRV = DestroyCondition(cvID);
	if (cvRV == -1) {
		print(" ->Fail to destroy a cv.\n");
	}else {
		print(" ->DestroyCondition cv %d.\n", cvID);
	}
	return 0;
}
