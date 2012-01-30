// problem2_waqChild.cc
// 

//----------------------------------------------------------------------
// Child Patient
//	A Child Patient must have an accompanying Parent. He/She listens to 
//	parent's instruction and do what their parents what them to do.
//	Child Patients are never abandoned by their Parent, nor go anywhere 
//	without their Parent.
//----------------------------------------------------------------------

void ChildPatient (unsigned int index) {
	int doctor_id;
	int patient_ID = index;
	
	fprintf(stdout, "Child Patient [%d] has entered the doctor's office with Parent [%d].", patient_ID);
	
	//child waits till his/her parent tells him/her what to do
	parentChildLock[patient_ID]->Acquire();
	parentChildSemaphore[patient_ID].V();
	parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);
	while (!leave[patient_ID]) {
		if (parentChildState[[patient_ID]] == P_EXAMXRAY) {		//needs an Xray
			doctor_id = parentTellChildDocID[patient_ID];
			fprintf(stdout, "Child Patient [%d] has been informed by Doctor [%d] that he needs an Xray.", patient_ID, doctor_id);
		}
		else if (parentChildState[[patient_ID]] == P_EXAMSHOT) {		//needs a shot
			doctor_id = parentTellChildDocID[patient_ID];
			fprintf(stdout, "Child Patient [%d] has been diagnosed by Doctor [%d].", patient_ID, doctor_id);
			fprintf(stdout, "Child Patient [%d] has been informed by Doctor [%d] that he will be administered a shot.", patient_ID, doctor_id);
		}
		else if (parentChildState[[patient_ID]] == P_EXAMDIAGNOISED) {		//finish diagnosing
			doctor_id = parentTellChildDocID[patient_ID];
			fprintf(stdout, "Child Patient [%d] has been diagnosed by Doctor [%d].", patient_ID, doctor_id);
		}
		else if (parentChildState[[patient_ID]] == P_TALBE) {		//get on the table
			fprintf(stdout, "Child Patient [%d] gets on the table.", patient_ID);
		}
		else if (parentChildState[[patient_ID]] == P_XRAY) {		//has been asked to take an xray
			fprintf(stdout, "Child Patient [%d] has been asked to take an Xray.", patient_ID);
		}
		parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
		parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);		//waits for parent's next instruction
	}	//end of while
	parentChildLock[patient_ID]->Release();		//child leaves with his/her parent
}




