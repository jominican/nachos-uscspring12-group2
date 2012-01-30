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
	int   doctorID;	// binded with the Doctor.
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
int numberOfWRNs;	// waiting room nurse.
int numberOfCashiers;
int numberOfXrays;
int numberOfAdults;
int numberOfChildren;

//----------------------------------------------------------------------
// Init
//	Initialize all of the global variables.
//
//----------------------------------------------------------------------

void Init()
{

}

//----------------------------------------------------------------------
// Doctor
//
//
//----------------------------------------------------------------------
int doctorWaitingCount[MAX_DOCTOR];
int doctorNurseIndex[MAX_DOCTOR];
Lock* examRoomLock;
Lock* examRoomLockArrry[MAX_NURSE];
Condition* examRoomCV;
Condition* examRoomCVArray[MAX_NURSE];
enum ExamRoom_State{
	E_BUSY, E_FREE, E_READY
};
enum ExamRoom_Task{
	E_FIRST, E_SECOND
};
examRoom_Task examRoomTask[MAX_NURSE];
ExamSheet* examRoomExamSheet[MAX_NURSE];	// the current Examination Sheet from the Patient in Doctor().
void Doctor(unsigned int index)
{
	while(true){
		// Check for the state of the Examination Rooms.
		doctorCheckingLock->Acquire();
		int roomIndex = -1;
		for(int i = 0; i < numberOfNurses; ++i){
			if(examRoomState[i] = E_READY){
				roomIndex = i; break;
			}
		}
		doctorCheckingLock->Release();
		
		// There is one room ready for the Doctor.
		if(roomIndex != -1){
			examRoomLockArray[roomIndex]->Acquire();
			// Wake up the waiting Patient and Nurse.
			examRoomDoctorID[roomIndex] = index;	// set the Doctor ID for the current Examination Room.
			examRoomCVArray[roomIndex]->Broadcast(examRoomLockArray[roomIndex]);
			examRoomCVArray[roomIndex]->Wait(examRoomLockArray[roomIndex]);
	
			if(E_FIRST == examRoomTask[roomIndex]){
				fprintf(stdout, "Doctor [%d] is reading the examination sheet of [Adult/Child] Patient [%d].\n", index, examRoomPatientID[roomIndex]);
				int result = rand() % 4;
				if(0 == result){	// need an Xray.
					int images = rand() % 3;
					fprintf(stdout, "Doctor [%d] notes down in the sheet that Xray is needed for [Adult/Child] Patient [%d].\n", index, examRoomPatientID[roomIndex]);
					examRoomExamSheet[index]->xray = true;
					examRoomExamSheet[index]->numberOfXray = images + 1;
				}else if(1 == result){	// need a shot.
					fprintf(stdout, "Doctor [%d] notes down in the sheet that [Adult/Child] Patient [%d] needs to be given a shot.\n", index, examRoomPatientID[roomIndex]);
					examRoomExamSheet[index]->shot = true;
				}else{	// fine and can leave.
					fprintf(stdout, "Doctor [%d] diagnoses [Adult/Child] Patient [%d] to be fine and is leaving the examination room.\n", index, examRoomPatientID[roomIndex]);
				}
				examRoomLockArray[roomIndex]->Signal(examRoomLockArray[roomIndex]);
			}else if(E_SECOND == examRoomTask[roomIndex]){
				fprintf(stdout, "Doctor [%d] is examining the Xrays of [Adult/Child] Patient [].\n", index);
				examRoomLockArray[roomIndex]->Wait(examRoomLockArray[roomIndex]);
				fprintf(stdout, "Doctor [%d] has left the examination room.", index);
				examRoomLockArray[roomIndex]->Signal(examRoomLockArray[roomIndex]);
			}
			
			examRoomLockArray[roomIndex]->Release();
		}else{	// No room is ready for the Doctor.
		
		}
	}
}

//----------------------------------------------------------------------
// Cashier
//
//
//----------------------------------------------------------------------
int cashierWaitingCount;
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
		
		fprintf(stdout, "Cashier receives the examination sheet from Adult Patient [].\n");
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier reads the examination sheet of Adult Patient [] and asks him to pay $....\n");
		cashierInteractCV->Signal(cashierInteractLock);
		fprintf(stdout, "Cashier accepts $.... from Adult Patient [].\n");
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier gives a receipt of $....... to Adult Patient [].\n");
		cashierInteractCV->Signal(cashierInteractLock);
		fprintf(stdout, "Cashier waits for the Patient to leave.\n");
		cashierInteractCV->Wait(cashierInteractLock);
		
		cashierState = C_FREE;
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
Lock* xrayWaitingLock[MAX_XRAY];
Lock* xrayInteractLock[MAX_XRAY];	// there could be MAX_XRAY (2) Xray Technicians.
Condition* xrayWaitingCV[MAX_XRAY];
Condition* xrayInteractCV[MAX_XRAY];
enum Xray_State{
	X_BUSY, X_FREE, X_FINISH
};
Xray_State xrayState[MAX_XRAY];
ExamSheet* xrayExamSheet[MAX_XRAY];	// the current Examination Sheet from the Patient in XrayTechnician().
void XrayTechnician(unsigned int index)
{
	while(true){
		/////////////////////////////////////////
		// Try to enter the Xray waiting section,
		// check for Xray Room state. One Xray Technician for each time.
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
		
		for(int i = 0; i < examSheetXray[index]->numberOfXray; ++i){
			if(0 == i)	fprintf(stdout, "Xray technician [%d] asks Adult Patient [] to get on the table.\n", index);
			else fprintf(stdout, "Xray Technician [%d] asks Adult Patient [] to move.\n", index);
			xrayInteractCV[index]->Signal(xrayInteractLock[index]);
			fprintf(stdout, "Xray Technician [%d] takes an Xray Image of Adult Patient [].\n", index);
			xrayInteractCV[index]->Wait(xrayInteractLock[index]);
			// Random the Xray result.
			int result = rand() % 3;
			if(0 == result) xrayExamSheet[index]->xrayImage[i] = "nothing";
			else if(1 == result) xrayExamSheet[index]->xrayImage[i] = "break";
			else if(2 == result) xrayExamSheet[index]->xrayImage[i] = "compound fracture";
			fprintf(stdout, "Xray Technician [%d] records [nothing/break/compound fracture] on Adult Patient []'s examination sheet.\n", index);
			xrayInteractCV[index]->Signal(xrayInteractLock[index]);
		}
		fprintf(stdout, "X-ray Technician [%d] calls Nurse [].\n", index);
		xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		fprintf(stdout, "X-ray Technician [%d] hands over examination sheet of Adult/Child Patient [] to Nurse [].\n", index);
		xrayInteractCV[index]->Signal(xrayInteractLock[index]);
		
		xrayState[index] = X_FREE;
		///////////////////////////////////////////////////////////////////////
		// Leave the Xray Technician interact section in XrayTechnician[index].
		// End of section 2 in XrayTechnician().
		xrayInteractLock[index]->Release();
	}
}

//----------------------------------------------------------------------
// Test1
//
//
//----------------------------------------------------------------------
void Test1()
{

}

//----------------------------------------------------------------------
// Test2
//
//
//----------------------------------------------------------------------
void Test2()
{

}

//----------------------------------------------------------------------
// Test3
//
//
//----------------------------------------------------------------------
void Test3()
{

}

//----------------------------------------------------------------------
// Test4
//
//
//----------------------------------------------------------------------
void Test4()
{

}

//----------------------------------------------------------------------
// Test5
//
//
//----------------------------------------------------------------------
void Test5()
{

}

//----------------------------------------------------------------------
// Test6
//
//
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
	Init();
	fprintf(stdout, "The Number of Doctors = %d\n", numberOfDoctors);
	fprintf(stdout, "The Number of Nurses = %d\n", numberOfNurses);
	fprintf(stdout, "The Number of Waiting Room Nurses = %d\n", numberOfWRNs);
	fprintf(stdout, "The Number of Cashiers = %d\n", numberOfCashiers);
	fprintf(stdout, "The Number of Xray Technicians = %d\n", numberOfXrays);
	fprintf(stdout, "The Number of Adult Patients = %d\n", numberOfAdults);
	fprintf(stdout, "The Number of Child Patients = %d\n", numberOfChildren);
	
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
		Thread* t = new Thread("");
		t->Fork(, i);
	}
	
	// Fork Child Patients.
	for(int i = 0; i < numberOfChildren; ++i){
		Thread* t = new Thread("");
		t->Fork(, i);
	}
}
