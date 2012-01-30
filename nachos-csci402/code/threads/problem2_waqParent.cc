// problem2_waqParent.cc
// 
#include "thread.h"
#include "synch.h"
#include "system.h"

int numberOfDoctors;
int numberOfNurses;
int numberOfWRNs;
int numberOfCashiers;
int numberOfXrays;
int numberOfAdults;
int numberOfChildren;

struct ExamSheet{
	int patientID;	// binded with the Patient.
	int age;
	char* name;
	int examRoomID;	// binded with the Examination Room.
	int xrayID;	// binded with the xray Room.
	bool xray;
	int numberOfXray;
	char* xrayImage[XRAY_IMAGE];	// 'nothing', 'break', 'compound fracture'.
	bool shot;
	float price;
	char* result;
};

#define MAX_PATIENT 30
#define MAX_DOCTOR 3
#define MAX_NURSE 5
#define MAX_XRAY 2
#define XRAY_IMAGE 3


Lock* WRLock;		//set the lock for entering the waiting room.
Lock* WRInteractLock;		//set the lock for the waiting room nurse and the patient to interact
Lock* patientWaitNurseLock;   //the lock of ready patient wait for the nurse to escort them
Lock* nurseLock[MAX_NURSE];		//the lock for the interaction between nurse and patient
Condition* patientWaitingCV;		//set the condition variable that patients are waiting for in line.
Condition* WRInteractCV;		//set the condition variable for the waiting room nurse and the patient to interact
Condition* patientWaitNurseCV;  // the condition variable for the monitor.
Condition* nurseCV[MAX_NURSE];		//the condition variable for the interaction between nurse and patient
Condition* nurseWrnCV;		//the condition variable for nurses waiting for the waiting room nurse
ExamSheet* wrnExamSheet;		//the waiting room nurse makes a new exam sheet for a new patient.
ExamSheet* examSheetArray[100];		//the exam sheet binded with different exam rooms
ExamSheet* xrayExamSheet[MAX_XRAY];	// the current Examination Sheet from the Patient in XrayTechnician().
int patientWaitingCount;		//counting how many patients are waiting in line.
int WRNurseTask;		//0 if the patient wants to get a sheet; 1 if the patient gives the sheet.
int nurseWaitPatientCount; //the number of nurses waiting for the patient;
int numOfNursesPassed; //in total, how many nurses have gone to take patients;
int patientWaitNurseCount; // the number of patients waiting for the nurse;
int nurseWaitWrnCount;		//the number of nurses waiting for the waiting room nurse
int nextActionForNurse; // the state to indicate a nurse is to take pick up a patient or not
int wrnNurseIndex;    //the id of the nurse who are interacting  with the patient in waitingRoom	
//int doctorNurseIndex[MAX_DOCTOR];    //the id of the nurse who are interacting  with the patient in waitingRoom	
bool isNurseInteracting;  //to indicate that if one nurse is interacting with a patient in the waiting room. initial value: 0					 
bool WRNurseState;		// false if the waiting room nurse is free and true for busy.
int indexOfSheet; //monitor variable: the index of the sheet kept in WRN. initial Value: 0
int indexOfPatient; //monitor variable: index Of Patient in the front of the waiting queue waiting for the nurse. initial value:0 
int wrnPatientID;		//???
int nurseTakeSheetID;
int examRoomDoctorID[MAX_NURSE];
int examRoomNurseID[MAX_NURSE];
int xrayRoomID[MAX_NURSE];
Lock* xrayWaitingLock[MAX_XRAY];
Lock* xrayInteractLock[MAX_XRAY];	// there could be MAX_XRAY (2) Xray Technicians.
Condition* xrayWaitingCV[MAX_XRAY];
Condition* xrayInteractCV[MAX_XRAY];
enum Xray_State{
	X_BUSY, X_FREE, X_FINISH
};
Xray_State xrayState[MAX_XRAY];
int xrayWaitingCount[MAX_XRAY];
int xrayPatientID[MAX_XRAY];
int examRoomSecondTimeID[MAX_XRAY];		//the id for the exam room in second visit   ??第一次拜访的id哪去了
enum ExamRoom_Task{
	E_FIRST, E_SECOND
};
examRoom_Task examRoomTask[MAX_NURSE];
int cashierWaitingCount;
int cashierPatientID;
Lock* cashierWaitingLock;
Lock* cashierInteractLock;	// there is just one single Cashier.
Condition* cashierWaitingCV;
Condition* cashierInteractCV;
enum Cashier_State{
	C_BUSY, C_FREE
};
Cashier_State cashierState;
ExamSheet* cashierExamSheet;	// the current Examination Sheet from the Patient in Cashier().
Lock* parentChildLock[MAX_PATIENT];
Lock* parentChildCV[MAX_PATIENT];
enum ParentChild_State{
	P_XRAY, P_TABLE, P_EXAMXRAY, P_EXAMSHOT, P_EXAMDIAGNOISED
};
ParentChild_State parentChildState[MAX_PATIENT];
int parentTellChildDocID[MAX_PATIENT];

bool leave[MAX_PATIENT];
Semaphore* parentChildSemaphore[MAX_PATIENT];

//----------------------------------------------------------------------
// Parent
//	Parent is similar with Patient, except that parent should pass the 
//	message to their child and let the child follow the instructions.
//	Parent is always accompanying with Child and never abandond them.
//----------------------------------------------------------------------

void Parent (unsigned int index) {
	
	ExamSheet* myExamSheet;		//the patient declares a new exam sheet for himself.
	char patient_id[4];
	char patient_name[13];
	int patient_ID = index;
	
	fprintf(stdout, "Adult Patient [%d] has entered the Doctor's Office.", patient_ID);
	
	//the patient gets in the line for the first time to get a form
	WRLock->Acquire();		//the patient tries to enter the waiting room and stay in line.
	fprintf(stdout, "Adult Patient [%d] gets in line of the Waiting Room Nurse to get registration form.", patient_ID);
	if (WRNurseState == true) {		// the waiting room nurse is busy. then the patient gets in line
		patientWaitingCount++;
		patientWaitingCV->Wait(WRLock);
	}else {
		WRNurseState = true;
	}
	WRLock->Release();		//the patient finishes waiting and gets ready to interact with the waiting room nurse.
	
	WRInteractLock->Acquire();
	//need to tell the waiting room nurse what to do -- firstly get a form.
	WRNurseTask = 0;
	wrnPatientID = patient_ID;
	WRInteractCV->Signal(WRInteractLock);		//tell the waiting room nurse that he comes
	WRInteractCV->Wait(WRInteractLock);		//wait for the waiting room nurse to make a new sheet
	fprintf(stdout, "[Adult patient/ Parent of child patient] get the form from the Waiting Room Nurse.");
	myExamSheet = wrnExamSheet;		//get the sheet
	WRInteractCV->Signal(WRInteractLock);		//tell the waiting room nurse that he gets the sheet.
	WRInteractLock->Release();
	
	//patient goes away and fills the form
	myExamSheet->patientID = patient_ID;		//write the patient id, his name, on the sheet.
	srand(time(NULL));
	myExamSheet->age = rand()%100+1;		//write the age of the patient on the sheet.
	sprintf(patient_id, "%d ", patient_ID);
	strcpy(patient_name, "Patient_");
	strcat(patient_name, patient_id);
	myExamSheet->name = patient_name;		//write the name string on the exam sheet struct for print
	
	//the patient gets back to the line to submit the form
	WRLock->Acquire();		//the patient tries to enter the waiting room and stay in line.
	fprintf(stdout, "Adult Patient [%d] gets in line of the Waiting Room Nurse to submit registration form.", patient_ID);
	if (WRNurseState == true) {		// the waiting room nurse is busy. then the patient gets in line
		patientWaitingCount++;
		patientWaitingCV->Wait(WRLock);
	}else {
		WRNurseState = true;
	}
	WRLock->Release();		//the patient finishes waiting and gets ready to interact with the waiting room nurse.
	
	WRInteractLock->Acquire();
	//need to tell the waiting room nurse what to do -- secondly turn over a form.
	WRNurseTask = 1;
	fprintf(stdout, "[Adult patient/ Parent of child patient] submit the filled form to the Waiting Room Nurse.");
	wrnExamSheet = myExamSheet;		
	WRInteractCV->Signal(WRInteractLock);		//turn over the filled form
	
	//patient waits for a nurse in line
	patientWaitNurseLock->Acquire();
	patientWaitNurseCount++;
	WRInteractLock->Release();		//patient leaves the waiting room nurse
	
	fprintf(stdout, "Parent [%d] waits for a Nurse to escort them to Exam room.", patient_ID);
	patientWaitNurseCV->Wait(patientWaitNurseLock);		//patient waits for a nurse come to escort him
	
	////////////////////////////////////////////
	// Parent and Child interaction

	fprintf(stdout, "Parent [%d] asks Child Patient [%d] to follow.", patient_ID);
	
	//
	////////////////////////////////////////////
	
	if(nurseWaitPatientCount > 0)
	{	//before leaving, wake up the next nurse in line to get prepared for escorting the next patient in line away
		nurseWaitPatientCV->Signal(patientWaitNurseLock);
		isNurseInteracting = BUSY;
	}
	else
	{
		isNurseInteracting = FREE;
	}
	
	//escorted away by the nurse
	int nurse_id = wrnNurseIndex;		//patient knows which nurse escorts him/her away
	patientWaitNurseLock->Release();
	
	//patient tries to acquire the lock for nurse
	nurseLock[nurse_id]->Acquire();
	patientID[nurse_id] = patient_ID;
	nurseExamSheet[nurse_id] = myExamSheet;		//patient把病例给nurse，防止之前赋值错误，重新赋值
	nurseCV[nurse_id]->Signal(nurseLock[nurse_id]);
	nurseCV[nurse_id]->Wait(nurseLock[nurse_id]);		//waiting for the nurse to get his exam sheet
	myExamSheet = nurseExamSheet[nurse_id];		//get information back from the nurse and know symptoms
	//nurseCV[nurse_id]->Signal(nurseLock[nurse_id]);
	nurseLock[nurse_id]->Release();
	
	//patient enters the exam room
	int examRoom_id = myExamSheet->examRoomID;
	examRoomLockArray[examRoom_id]->Acquire();		//必须要nurse先acquire到这个锁
	examRoomLock->Acquire();
	examRoomState[examRoom_id] = E_READY;
	examRoomTask[examRoom_id] = E_FIRST;
	examRoomLock->Release();
	examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);		//get into a exam room and wait for a doctor, together with a nurse
	
	fprintf(stdout, "Adult Patient/Parent [%d] says, “My/His symptoms are Pain/Nausea/Hear Alien Voices”.", patient_ID)
	examRoomExamSheet[examRoom_id] = myExamSheet;		//turn over the exam sheet to the doctor
	examRoomCVArray[examRoom_id]->Signal(examRoomLockArray[examRoom_id]);		
	examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);		
	myExamSheet = examRoomExamSheet[examRoom_id];		//doctor finishes the first time examination
	
	if (myExamSheet->xray) {		// the patient needs to take an Xray
		
		////////////////////////////////////////////
		// Parent and Child interaction
		//解决parent和child谁先拿锁的问题 pv
		
		parentChildSemaphore[patient_ID].P();
		parentChildLock[patient_ID]->Acquire();
		parentChildState[patient_ID] = P_EXAMXRAY;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoom_id];
		parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
		parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);
		parentChildLock[patient_ID]->Release();
		
		//
		////////////////////////////////////////////
		
		examRoomLock->Acquire();		//这是个公共大锁，有可能其他nurse在抢锁去查看exam room的状态，导致patient很久才acquire到这个锁
		examRoomState[examRoom_id] = E_FINISH;
		examRoomLock->Release();
		fprintf(stdout, "Adult Patient [%d] waits for a Nurse to escort them to Xray room.", patient_ID);
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);			//the patient waits for a nurse to escort him/her to xray room
		
		//a nurse comes to escort patient away
		int examRoomNurse_id = examRoomNurseID[examRoom_id];		//record the nurse who takes patient away from the exam room for the first time
		int xrayRoom_id = xrayRoomID[examRoom_id];		//the xray room he needs to go to
		examRoomLockArray[examRoom_id]->Release();	
		
		xrayWaitingLock[xrayRoom_id]->Acquire;		
		if (xrayState[xrayRoom_id] == X_BUSY) {		// the Xray technician is busy. then the patient gets in line
			xrayWaitingCount[xrayRoom_id];++;
			xrayWaitingCV[xrayRoom_id]->Wait(xrayWaitingLock[xrayRoom_id]);
		}else {
			xrayState[xrayRoom_id] == X_BUSY;
		}
		xrayWaitingLock[xrayRoom_id]->Release();	
		
		//the patient enters the xray room
		xrayInteractLock[xrayRoom_id]->Acquire();
		fprintf(stdout, "Nurse [%d] informs X-Ray Technician [%d] about Adult/Child Patient [%d] and hands over the examination sheet.", examRoomNurse_id, xrayRoom_id, patient_ID);
		fprintf(stdout, "Nurse [%d] leaves the X-ray Room [%d].", examRoomNurse_id, xrayRoom_id);
		xrayExamSheet[xrayRoom_id] = myExamSheet;
		xrayPatientID[xrayRoom_id] = patient_ID;
		xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);		//the patient gives the exam sheet to the xray technician
		
		//start to take xrays
		int i = 0;
		while (i < myExamSheet->numberOfXray) {
			xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);
			if (i == 0) {
				
				////////////////////////////////////////////
				// Parent and Child interaction
				//解决parent和child谁先拿锁的问题
				
				parentChildLock[patient_ID]->Acquire();
				parentChildState[[patient_ID]] = P_TABLE;
				parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
				parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);
				parentChildLock[patient_ID]->Release();
				
				//
				////////////////////////////////////////////
				
			}
			xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);
			xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);
			fprintf(stdout, "Adult Patient [%d] has been asked to take an Xray.", patinet_ID);
			xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);
			i++;
		}		//xray technician finishes taking xrays for the patient
		
		fprintf(stdout, "Adult Patient [%d] waits for a Nurse to escort him/her to exam room", patinet_ID);
		xrayCheckingLock->Acquired();		//同样抢锁的问题
		xrayState[xrayRoom_id] == X_FINISH;
		xrayCheckingLock->Release();
		xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);		//the patient is waiting for a nurse to take him/her back to the exam room

		//a second nurse comes to escort patient away back to exam room
		int examRoomSecondTime_id = examRoomSecondTimeID[xrayRoom_id];
		xrayInteractLock[xrayRoom_id]->Release();
		examRoomLockArray[examRoomSecondTime_id]->Acquire();
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		examRoomLock->Acquire();		//同样抢锁的问题
		examRoomState[examRoomSecondTime_id] = E_READY;
		examRoomTask[examRoomSecondTime_id] = E_SECOND;
		examRoomLock->Release();
		examRoomCVArray[examRoomSecondTime_id]->Wait(examRoomLockArray[examRoomSecondTime_id]);		//get into a exam room and wait for a doctor, together with a nurse
		
		////////////////////////////////////////////
		// Parent and Child interaction
		//解决parent和child谁先拿锁的问题
		
		parentChildLock[patient_ID]->Acquire();
		parentChildState[[patient_ID]] = P_EXAMDIAGNOISED;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoomSecondTime_id];
		parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
		parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);
		parentChildLock[patient_ID]->Release();
		
		//
		////////////////////////////////////////////
		
		examRoomLock->Acquire();
		examRoomState[examRoomSecondTime_id] = E_FINISH;
		examRoomLock->Release();
		//the patient waits for a third nurse to take him/her to the cashier
		examRoomCVArray[examRoomSecondTime_id]->Wait(examRoomLockArray[examRoomSecondTime_id]);
		examRoomCVLock[examRoomSecondTime_id]->Release();
	}
	else if (myExamSheet->shot) {		//the patient needs a shot
		
		////////////////////////////////////////////
		// Parent and Child interaction
		//解决parent和child谁先拿锁的问题
		
		parentChildSemaphore[patient_ID].P();
		parentChildLock[patient_ID]->Acquire();
		parentChildState[[patient_ID]] = P_EXAMSHOT;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoom_id];
		parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
		parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);
		parentChildLock[patient_ID]->Release();
		
		//
		////////////////////////////////////////////
		
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FINISH;
		examRoomLock->Release();
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);
		
		fprintf(stdout, "Adult Patient/Parent [%d] says, “Yes I am/He is ready for the shot”.", patinet_ID);
		examRoomCVArray[examRoom_id]->Signal(examRoomLockArray[examRoom_id]);
		//the patient waits for the nurse to take him/her to the cashier
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);
		examRoomCVLock[examRoom_id]->Release();
	}
	else {		//the patient has no problem and can go to the cashier directly
		
		////////////////////////////////////////////
		// Parent and Child interaction
		//解决parent和child谁先拿锁的问题
		
		parentChildSemaphore[patient_ID].P();
		parentChildLock[patient_ID]->Acquire();
		parentChildState[[patient_ID]] = P_EXAMDIAGNOISED;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoom_id];
		parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
		parentChildCV[patient_ID]->Wait(parentChildLock[patient_ID]);
		parentChildLock[patient_ID]->Release();
		
		//
		////////////////////////////////////////////
		
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FINISH;
		examRoomLock->Release();
		//the patient waits for a second nurse to take him/her to the cashier
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);
		examRoomCVLock[examRoom_id]->Release();
	}

	//patient is escorted to the cashier.
	cashierWaitingLock->Acquire();		
	if (cashierState == C_BUSY) {		
		cashierWaitingCount;++;
		fprintf(stdout, "Adult Patient [%d] enters the queue for Cashier.", patinet_ID);
		cashierWaitingCV->Wait(cashierWaitingLock);
	}else {
		cashierState == C_BUSY;
	}
	cashierWaitingLock->Release();
	
	//interact with the cashier
	cashierInteractLock->Acquire();
	fprintf(stdout, "Adult Patient [%d] reaches the Cashier.", patinet_ID);
	fprintf(stdout, "Adult Patient [%d] hands over his examination sheet to the Cashier.", patinet_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractCV->Wait(cashierInteractLock);	
	fprintf(stdout, "Adult Patient [%d] pays the Cashier $402.", patinet_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractCV->Wait(cashierInteractLock);
	fprintf(stdout, "Adult Patient [%d] receives a receipt from the Cashier.", patinet_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractCV->Wait(cashierInteractLock);
	
	//patient finishes everything and leaves
	fprintf(stdout, "Adult Patient [%d] leaves the doctor's office.", patinet_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractLock->Release();
	
	////////////////////////////////////////////
	// Parent and Child interaction
	//解决parent和child谁先拿锁的问题
	
	parentChildLock[patient_ID]->Acquire();
	leave[patient_ID] = true;
	parentChildCV[patient_ID]->Signal(parentChildLock[patient_ID]);
	parentChildLock[patient_ID]->Release();
	
	//parent leaves the doctor's office with the child
	////////////////////////////////////////////
	
}
