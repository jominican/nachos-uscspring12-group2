#include "syscall.h"
#include "print.h"

/*Test Destroy Condition*/

int cvID;
int cvRV;

int main()
{
	
	print("Test Destroy CV\n");
	
	cvID = CreateCondition("cv1", 3);
	print("Create cv %d.\n", cvID);
	
	cvRV = DestroyCondition(cvID);
	if (cvRV == -1) {
		print("Fail to destroy a cv.\n");
	}else {
		print("DestroyLock cv %d.\n", cvID);
	}
	return 0;
}
