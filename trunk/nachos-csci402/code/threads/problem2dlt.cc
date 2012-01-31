#include "thread.h"
#include "synch.h"
#include "system.h"

// Constants for corresponding entities. 
#define MAX_DOCTOR 3
#define MAX_NURSE 5
#define MAX_XRAY 2
#define XRAY_IMAGE 3

// Struct for Examination Sheet.
struct ExamSheet{
	int   patientID;	// binded with the Patient.
	int   age;
	char* name;
	bool  xray;
	int   xrayID;	// binded with the Xray Technician.
	int   numberOfXray;
	char* xrayImage[XRAY_IMAGE];	// 'nothing', 'break', 'compound fracture'.
	bool  shot;
	float price;
	char* result;	// examination result.
};

int numberOfDoctors;
int numberOfNurses;
int numberOfExamRooms;
int numberOfWRNs;	// waiting room nurse (1).
int numberOfCashiers;	// 1.
int numberOfXrays;
int numberOfPatients;	// Patients = Adults + Children.
int numberOfAdults;
int numberOfChildren;
int numberOfParents;

int* childToParent;
//----------------------------------------------------------------------
// Init
//	Initialize all of the global variables.
//	Locks.
//	Condition Variables.
//	Monitor Variables.
//	Entity State.
//----------------------------------------------------------------------
void Init()
{
	// Initialize the Doctor().
	examRoomLock = new Lock("examRoomLock");
	examRoomCheckingLock = new Lock("examRoomCheckingLock");
	for(int i = 0; i < MAX_NURSE; ++i){
		examRoomLockArray[i] = new Lock("examRoomLock");
	}
	examRoomCV = new Condition("examRoomCV");
	for(int i = 0; i < MAX_NURSE; ++i){
		examRoomCVArray[i] = new Condition("examRoomCVArray");
	}
	for(int i = 0; i < MAX_NURSE; ++i){
		examRoomState[i] = E_FREE;
	}
	for(int i = 0; i < MAX_NURSE; ++i){
		examRoomTask[i] = E_FIRST;
	}
	
	// Initialize the Cashier().
	cashierWaitingCount = 0;
	cashierWaitingLock = new Lock("cashierWaitingLock");
	cashierInteractLock = new Lock("cashierInteractLock");
	cashierWaitingCV = new Condition("cashierWaitingCV");
	cashierInteractCV = new Condition("cashierInteractCV");
	cashierState = C_FREE;
	
	// Initialize the XrayTechnician().
	for(int i = 0; i < MAX_XRAY; ++i){
		xrayWaitingCount[i] = 0;
	}	
	xrayCheckingLock = new Lock("xrayCheckingLock");
	for(int i = 0; i < MAX_XRAY; ++i){
		xrayWaitingLock[i] = new Lock("xrayWaitingLock");
	}
	for(int i = 0; i < MAX_XRAY; ++i){
		xrayInteractLock[i] = new Lock("xrayInteractLock");
	}
	for(int i = 0; i < MAX_XRAY; ++i){
		xrayWaitingCV[i] = new Condition("xrayWaitingCV");
	}
	for(int i = 0; i < MAX_XRAY; ++i){
		xrayInteractCV[i] = new Condition("xrayInteractCV");
	}
	for(int i = 0; i < MAX_XRAY; ++i){
		xrayState[i] = X_FREE;
	}
	
	// Initialize the Waiting Room Nurse().
	
	// Initialize the Nurse().
	
	// Initialize the Patient().
	childToParent = new int[numberOfPatients];
	
	// Initialize the Parent().
}

//----------------------------------------------------------------------
// Doctor
//
//
//----------------------------------------------------------------------
int examRoomDoctorID[MAX_NURSE];
int examRoomPatientID[MAX_NURSE];
Lock* examRoomLock;
Lock* examRoomCheckingLock;	// for Doctors to check the E_READY of the Examination Room.
Lock* examRoomLockArry[MAX_NURSE];
Condition* examRoomCV;
Condition* examRoomCVArray[MAX_NURSE];
enum ExamRoom_State{
	E_BUSY, E_FREE, E_READY, E_FINISH	// E_READY: the Examination Room is ready for Doctor; E_FINISH: the Examination Room is ready (finish) for Nurse.
};
enum ExamRoom_Task{
	E_FIRST, E_SECOND	// the first or the second time the Patient meets with the Doctor.
};
ExamRoom_State examRoomState[MAX_NURSE];
ExamRoom_Task examRoomTask[MAX_NURSE];
ExamSheet* examRoomExamSheet[MAX_NURSE];	// the current Examination Sheet from the Patient in Doctor().
void Doctor(unsigned int index)
{
	while(true){
		// Check for the state of the Examination Rooms.
		examRoomCheckingLock->Acquire();
		int roomIndex = -1;
		for(int i = 0; i < numberOfNurses; ++i){
			if(E_READY == examRoomState[i]){
				roomIndex = i; break;
			}
		}
		examRoomCheckingLock->Release();
		
		// There is one room ready for the Doctor.
		if(roomIndex != -1){
			examRoomLockArray[roomIndex]->Acquire();
			// Wake up the waiting Patient and Nurse.
			examRoomDoctorID[roomIndex] = index;	// set the Doctor ID for the current Examination Room.
			examRoomCVArray[roomIndex]->Broadcast(examRoomLockArray[roomIndex]);
			examRoomCVArray[roomIndex]->Wait(examRoomLockArray[roomIndex]);	// wait to be waked up by the Patient.
	
			if(E_FIRST == examRoomTask[roomIndex]){
				fprintf(stdout, "Doctor [%d] is reading the examination sheet of [Adult/Child] Patient [%d].\n", index, examRoomPatientID[roomIndex]);
				int result = rand() % 4;
				if(0 == result){	// need an Xray.
					int images = rand() % 3;
					fprintf(stdout, "Doctor [%d] notes down in the sheet that Xray is needed for [Adult/Child] Patient [%d].\n", index, examRoomPatientID[roomIndex]);
					examRoomExamSheet[roomIndex]->xray = true;
					examRoomExamSheet[roomIndex]->numberOfXray = images + 1;
				}else if(1 == result){	// need a shot.
					fprintf(stdout, "Doctor [%d] notes down in the sheet that [Adult/Child] Patient [%d] needs to be given a shot.\n", index, examRoomPatientID[roomIndex]);
					examRoomExamSheet[roomIndex]->shot = true;
				}else{	// fine and can leave.
					fprintf(stdout, "Doctor [%d] diagnoses [Adult/Child] Patient [%d] to be fine and is leaving the examination room.\n", index, examRoomPatientID[roomIndex]);
				}
				examRoomLockArray[roomIndex]->Signal(examRoomLockArray[roomIndex]);
			}else if(E_SECOND == examRoomTask[roomIndex]){
				fprintf(stdout, "Doctor [%d] is examining the Xrays of [Adult/Child] Patient [%d].\n", index, examRoomPatientID[roomIndex]);
				fprintf(stdout, "Doctor [%d] has left the examination room.\n", index);
				examRoomLockArray[roomIndex]->Signal(examRoomLockArray[roomIndex]);
			}
			// Loop into the next iteration.
			examRoomLockArray[roomIndex]->Release();
		}else{	// No room is ready for the Doctor.
			// Avoid busy waiting.
			currentThread->Yield();
		}
	}
}

//----------------------------------------------------------------------
// Cashier
//
//
//----------------------------------------------------------------------
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
void Cashier(unsigned int index)
{
	while(true){
		////////////////////////////////////////////
		// Try to enter the Cashier waiting section.
		// There may be some Patients waiting for the Cashier.
		// Start of section 1 in Cashier().
		cashierWaitingLock->Acquire();
		if(cashierWaitingCount > 0){
			// Some Patients are in line.
			// Wake them up.
			cashierWaitingCV->Signal(cashierWaitingLock);
			cachierWaitingCount--;
			cashierState = C_BUSY;
		}else{
			// No patients are in line.
			cachierState = C_FREE;
		}
		/////////////////////////////////////////////
		// Try to enter the Cashier interact section. 
		// There may be one Patient to interact with the Cashier.
		// Start of section 2 in Cashier().
		cashierInteractLock->Acquire();
		/////////////////////////////////////
		// Leave the Cashier waiting section.
		// End of section 1 in Cashier().
		cashierWaitingLock->Release();
		
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier receives the examination sheet from Adult Patient [%d].\n", cashierPatientID);
		fprintf(stdout, "Cashier reads the examination sheet of Adult Patient [%d] and asks him to pay $%f\n", cashierPatientID, cashierExamSheet->price);
		cashierInteractCV->Signal(cashierInteractLock);
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier accepts $%f from Adult Patient [%d].\n", cashierExamSheet->price, cashierPatientID);
		fprintf(stdout, "Cashier gives a receipt of $%f to Adult Patient [%d].\n", cashierPatientID, cashierExamSheet->price);
		cashierInteractCV->Signal(cashierInteractLock);
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier waits for the Patient to leave.\n", cashierPatientID);
		cashierInteractCV->Signal(cashierInteractLock);
		cashierInteractCV->Wait(cashierInteractLock);
		
		//////////////////////////////////////
		// Leave the Cashier interact section.
		// End of section 2 in Cashier().
		cashierInteractLock->Release();
	}
}

//----------------------------------------------------------------------
// XrayTechnician
//
//
//----------------------------------------------------------------------
int xrayWaitingCount[MAX_XRAY];
int xrayPatientID[MAX_XRAY];
Lock* xrayCheckingLock;
Lock* xrayWaitingLock[MAX_XRAY];
Lock* xrayInteractLock[MAX_XRAY];	// there could be MAX_XRAY (2) Xray Technicians.
Condition* xrayWaitingCV[MAX_XRAY];
Condition* xrayInteractCV[MAX_XRAY];
enum Xray_State{
	X_BUSY, X_FREE, X_READY, X_FINISH
};
Xray_State xrayState[MAX_XRAY];
ExamSheet* xrayExamSheet[MAX_XRAY];	// the current Examination Sheet from the Patient in XrayTechnician().
void XrayTechnician(unsigned int index)
{
	while(true){
		/////////////////////////////////////////
		// Try to enter the Xray waiting section.
		// Start of section 1 in XrayTechnician().
		xrayWaitingLock[index]->Acquire();
		// There is one room waiting for Xray Technician.
		if(xrayWaitingCount[index] > 0){
			xrayWaitingCV[index]->Signal(xrayWaitingLock[index]);
			xrayWaitingCount[index]--;
			xrayState[index] = X_BUSY;
		}else{
			xrayState[index] = X_FREE;
		}
		//////////////////////////////////////////
		// Try to enter the Xray interact section.
		// Start of section 2 in XrayTechnician().
		xrayInteractLock[index]->Acquire();
		//////////////////////////////////
		// Leave the Xray waiting section.
		// End of section 1 in XrayTechnician().
		xrayWaitingLock[index]->Release();
		
		xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		for(int i = 0; i < examSheetXray[index]->numberOfXray; ++i){
			if(0 == i)	fprintf(stdout, "Xray technician [%d] asks Adult Patient [%d] to get on the table.\n", index, xrayPatientID[index]);
			else fprintf(stdout, "Xray Technician [%d] asks Adult Patient [%d] to move.\n", index, xrayPatientID[index]);
			xrayInteractCV[index]->Signal(xrayInteractLock[index]);
			xrayInteractCV[index]->Wait(xrayInteractLock[index]);
			fprintf(stdout, "Xray Technician [%d] takes an Xray Image of Adult Patient [%d].\n", index, xrayPatientID[index]);
			// Random the Xray result.
			int result = rand() % 3;
			if(0 == result) xrayExamSheet[index]->xrayImage[i] = "nothing";
			else if(1 == result) xrayExamSheet[index]->xrayImage[i] = "break";
			else if(2 == result) xrayExamSheet[index]->xrayImage[i] = "compound fracture";
			fprintf(stdout, "Xray Technician [%d] records [nothing/break/compound fracture] on Adult Patient [%d]'s examination sheet.\n", index, xrayPatientID[index]);
			xrayInteractCV[index]->Signal(xrayInteractLock[index]);
			xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		}
		fprintf(stdout, "X-ray Technician [%d] calls Nurse [%d].\n", index);
		xrayInteractCV[index]->Signal(xrayInteractLock[index]);
		xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		fprintf(stdout, "X-ray Technician [%d] hands over examination sheet of Adult/Child Patient [%d] to Nurse [%d].\n", index, xrayPatientID[index]);
		
		///////////////////////////////////////////////////////////////////////
		// Leave the Xray Technician interact section in XrayTechnician[index].
		// End of section 2 in XrayTechnician().
		xrayInteractLock[index]->Release();
	}
}

//----------------------------------------------------------------------
// Test1
//	Test goal: Child Patients are never abandoned by their Parent, nor go 
//	anywhere without their Parent.
//----------------------------------------------------------------------
void Test1()
{

}

//----------------------------------------------------------------------
// Test2
//	Test goal: Waiting Room Nurses only talk to 
//	one Patient/Parent at a time.
//----------------------------------------------------------------------
void Test2()
{

}

//----------------------------------------------------------------------
// Test3
//	Test goal: Cashiers only talk to one Patient/Parent at a time.
//
//----------------------------------------------------------------------
void Test3()
{

}

//----------------------------------------------------------------------
// Test4
//	Test goal: Patients/Parents never go anywhere 
//	without being escorted by a Nurse.
//----------------------------------------------------------------------
void Test4()
{

}

//----------------------------------------------------------------------
// Test5
//	Test goal: All Patients leave the Doctor's Office. No one stays in 
//	the Waiting Room, Examination Room, or Xray Room, forever.
//----------------------------------------------------------------------
void Test5()
{

}

//----------------------------------------------------------------------
// Test6
//	Test goal: Two Doctors never examine the same Patient 
//	at the same time.
//----------------------------------------------------------------------
void Test6()
{

}

//----------------------------------------------------------------------
// Problem2
//
//
//----------------------------------------------------------------------
void Problem2()
{
	// Initialize the Locks, Condition Variables, Monitor Variables
	// and the initial state of each entity.
	Init();
	
	fprintf(stdout, "Choose the test case for the part 2.\n");
	fprintf(stdout, "For the System Test, press 0.\n");
	fprintf(stdout, "For the Simple Test for the requirement 1-6, press the corressponding number from 1-6.\n");
	int input = -1;
	scanf("%d", &input);
	switch(input){
		case 1: Test1(); break;
		case 2: Test2(); break;
		case 3: Test3(); break;
		case 4: Test4(); break;
		case 5: Test5(); break;
		case 6: Test6(); break;
	}
	if(input != 0) return;	// when input is 0, System Test.
	
	// Wait for the input from the command line.
	fprintf(stdout, "Number of Doctors = ?\n");
	scanf("%d", &numberOfDoctors);
	fprintf(stdout, "Number of Nurses = ?\n");
	scanf("%d", &numberOfNurses);
	fprintf(stdout, "Number of XRay Technicians = ?\n");
	scanf("%d", &numberOfXrays);
	fprintf(stdout, "Number of Patients = ?\n");
	scanf("%d", &numberOfPatients);
	
	// Initialize the numbers of all entities.
	numberOfExamRooms = numberOfNurses;
	numberOfWRNs = numberOfCashier = 1;
	numberOfAdults = numberOfPatients / 2; // make the numbers of Adult Patients and Child Patients as equal as possible.
	numberOfChildren = numberOfParents = numberOfPatients - numberOfAdults;
	
	// Check the validation of the input.
	if(numberOfDoctors != 2 && numberOfDocrots != 3){
		fprintf(stdout, "The interval of the number of the Doctors is [2, 3].\n");
		return;
	}
	if(numberOfNurses < 2 || numberOfNurses > 5){
		fprintf(stderr, "The interval of the number of the Nurses is [2, 5].\n");
		return;
	}
	if(numberOfXrays != 1 || numberOfXray != 2){
		fprintf(stderr, "The interval of the number of the Xray Technicians is [1, 2].\n");
		return;
	}
	if(numberOfPatients < 30){
		fprintf(stderr, "The interval of the number of the Patients is [30, MAX_INTEGER).\n");
		return;
	}
	
	// Fork Doctors.
	for(int i = 0; i < numberOfDoctors; ++i){
		Thread* t = new Thread("Doctor");
		t->Fork(Doctor, i);
	}
	
	// Fork Nurses.
	for(int i = 0; i < Nurses; ++i){
		Thread* t = new Thread("Nurse");
		t->Fork(Nurse, i);
	}
	
	// Fork Waiting Room Nurses.
	for(int i = 0; i < numberOfWRNs; ++i){
		Thread* t = new Thread("Waiting Room Nurse");
		t->Fork(WaitingRoomNurse, i);
	}
	
	// Fork Cashiers.
	for(int i = 0; i < numberOfCashiers; ++i){
		Thread* t = new Thread("Cashier");
		t->Fork(Cashier, i);
	}
	
	// Fork Xray Technicians.
	for(int i = 0; i < numberOfXrays; ++i){
		Thread* t = new Thread("XrayTechnician");
		t->Fork(XrayTechnician, i);
	}
	
	// Fork Adult Patients.
	for(int i = 0; i < numberOfAdults; ++i){
		Thread* t = new Thread("Patient");
		t->Fork(Patient, i);
	}
	
	// Fork Child Patients.
	for(int i = 0; i < numberOfChildren; ++i){
		Thread* t = new Thread("Child");
		t->Fork(Child, i);
	}
	
	// Fork Parent.
	for(int i = 0; i < numberOfParents; ++i){
		Thread* t = new Thread("Parent");
		t->Fork(Parent, i);
	}
	
	//////////////////
	// Wait to finish.
}
