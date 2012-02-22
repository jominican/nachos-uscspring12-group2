#include "syscall.h"
#include "print.h"

/*
Test CreateCondition_Syscall
*/

int cvID;
int cvRV;

int main()
{
	print("Test CreateCondition() system call.\n");
	
	cvID = CreateCondition("cv1", 3);
	if (cvID == -1) {
		print(" ->Fail to create a cv.\n");
	}else {
		print(" ->Create cv %d.\n", cvID);
		
		cvRV = DestroyCondition(cvID);
		print(" ->DestroyCondition cv %d.\n", cvID);
	}
	return 0;
}
