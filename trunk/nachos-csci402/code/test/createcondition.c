#include "syscall.h"
#include "print.h"

/*Test Create Condition*/

int cvID;
int cvRV;

int main()
{
	
	print("Test Create CV\n");
	
	cvID = CreateCondition("cv1", 3);
	if (cvID == -1) {
		print("Fail to create a cv.\n");
	}else {
		print("Create cv %d.\n", cvID);
		
		cvRV = DestroyCondition(cvID);
		print("DestroyLock cv %d.\n", cvID);
	}
	return 0;
}
