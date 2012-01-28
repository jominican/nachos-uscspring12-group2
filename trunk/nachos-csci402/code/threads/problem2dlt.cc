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
	int   examRoomID;	// binded with the Examination Room.
	bool  xray;
	int   numberOfXray;
	char* xrayImage[XRAY_IMAGE];	// 'nothing', 'break', 'compound fracture'.
	bool  shot;
	float price;
	char* result;	// examination result.
};

int numberOfDoctors;
int numberOfNurses;
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

void init()
{

}

//----------------------------------------------------------------------
// Doctor
//
//
//----------------------------------------------------------------------
int doctorWaitingCount[MAX_DOCTOR];
Lock* doctorWaitingLock[MAX_DOCTOR];
Lock* doctorInteractLock[MAX_DOCTOR];
Condition* doctorWaitingCV[MAX_DOCTOR];
Condition* doctorWaitingCV[MAX_DOCTOR];
enum Doctor_State{
	D_BUSY, D_FREE
};
enum Doctor_Task{
	T_FIRST, T_SECOND
};
Doctor_State doctorState[MAX_DOCTOR];
Doctor_Task doctorTask[MAX_DOCTOR];
void Doctor(unsigned int index)
{
	while(true){
		///////////////////////////////////////////
		// Try to enter the Doctor waiting section.
		// There may be one Patient waiting for the Doctor.
		// Start of section 1 in Doctor();
		doctorWaitingLock[index]->Acquire();
		if(doctorWaitingCount[index] > 0){
			// One Patient already here.
			// Wake him up.
			doctorWaitingCV[index]->Signal(doctorWaitingLock[index]);
			doctorWaitingCount--;
			doctorState[index] = D_BUSY;
		}else{
			// No Patient is here.
			doctorState[index] = D_FREE;
		}
		////////////////////////////////////////////
		// Try to enter the Doctor interact section.
		// There may be one Patient to interact with the Doctor.
		// Start of section 2 in Doctor().
		doctorInteractLock[index]->Acquire();
		// Leave the Doctor waiting section.
		// End of section 2 in Doctor().
		doctorWaitingLock[index]->Release();
		
		//////////////////////////////////////////
		// Begin the interaction with one Patient.
		// 2 different kinds of task, T_FIRST (the first time the Doctor sees the Patient), 
		// T_SECOND (the second time the Doctor sees the Patient).
		// Task 1:
		// 	1: Doctor waits for the Nurse to inform there is a Patient ready to be examined.
		// 	2: Doctor reads the Examination Sheet and randomly gives Xray or shot. 
		//	3:  
		// Task 2:
		if(T_FIRST == doctorTask[index]){
			fprintf(stdout, "Doctor waits for the Nurse to inform there is a Patient ready to be examined.\n");
			doctorInteractCV[index]->Wait(doctorInteractLock[index]);
			ExamSheet* examSheet; 
			int result = rand() % 4;
			if(0 == result){	// need an Xray.
				int images = rand() % 3;
				fprintf(stdout, "Doctor tells the Patient to have an Xray.\n");
				examSheet->xray = true;
				examSheet->numberOfXray = images + 1;
			}else if(1 == result){	// need a shot.
				fprintf(stdout, "Doctor tells the Patient to have a shot.\n");
				examSheet->shot = true;
			}else{	// fine and can leave.
				fprintf(stdout, "Doctor tells the Patient he is fine and can leave.\n");
			}
			doctorInteractCV[index]->Signal(doctorInteractLock[index]);
		}else if(T_SECOND == doctorTask[index]){
			fprintf(stdout, "Doctor waits for the Nurse to give the Examination Sheet.\n");
			doctorInteractCV[index]->Wait(doctorInteractLock[index]);
			fprintf(stdout, "Doctor tells the results to the Patient.\n");
			doctorInteractCV[index]->Signal(doctorInteractLock[index]);
		}
		
		/////////////////////////////////////
		// Leave the Doctor interact section.
		// End of section 2 in Doctor().
		doctorInteractLock[index]->Release(doctorInteractLock[index]);
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
		
		//////////////////////////////////////////
		// Begin the interaction with one Patient.
		// 1: Cashier waits for the Examination Sheet from the Patient.
		// 2: Cashier reads and determines the Patient's bill, and tell the 
		//    Patient how much should pay.
		// 3: Cashier waits for the Patient to pay.
		// 4: Cashier gives the Patient a receipt.
		// 5: Cashier waits for the Patient to leave.
		fprintf(stdout, "Cashier waits for the Examination Sheet from the Patient.\n");
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier reads and determines the Patient's bill, and\
				tell the Patient how much should pay.\n");
		cashierInteractCV->Signal(cashierInteractLock);
		fprintf(stdout, "Cashier waits for the Patient to pay.\n");
		cashierInteractCV->Wait(cashierInteractLock);
		fprintf(stdout, "Cashier gives the Patient a receipt.\n");
		cashierInteractCV->Signal(cashierInteractLock);
		fprintf(stdout, "Cashier waits for the Patient to leave.\n");
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
Lock* xrayWaitingLock[MAX_XRAY];
Lock* xrayInteractLock[MAX_XRAY];	// there could be MAX_XRAY (2) Xray Technicians.
Condition* xrayWaitingCV[MAX_XRAY];
Condition* xrayInteractCV[MAX_XRAY];
enum Xray_State{
	X_BUSY, X_FREE
};
Xray_State xrayState[MAX_XRAY];
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
		
		//////////////////////////////////////////
		// Begin the interaction with one Patient.
		// 1: Xray Technician waits for the Examination Sheet from the Nurse.
		// 2: Xray Technician gets the Examinatin Sheet.
		// 3: Xray Technician waits the Nurse to leave.
		// 4: Xray Technician asks the Patient to get onto the table.
		// 5: Xray Technician waits the Patient to get onto the table.
		// 6: Xray Technician takes an Xray.
		// 7: If possible, iterate from 4 to 6.
		// 8: Xray Technician waits for the Nurse to come.
		// 9: Xray Technician gives the Examination Sheet to the Nurse.
		// 10: Xray Technician waits for the Nurse and Patient to leave the Xray room.
		fprintf(stdout, "Xray Technician waits for the Examination Sheet from the Nurse.\n");
		xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		ExamSheet* examSheet; 
		fprintf(stdout, "Xray Technician gets the Examination Sheet.\n");
		xrayInteractCV[index]->Signal(xrayInteractLock[index]);
		fprintf(stdout, "Xray Technician waits the Nurse to leave.\n");
		xrayInteractCV[index]->Wait(xrayInteractLock);
		for(int i = 0; i < examSheet->numberOfXray; ++i){
			if(0 == i)	fprintf(stdout, "Xray Technician asks the Patient to get onto the table.\n");
			else fprintf(stdout, "Xray Technician asks the Patient to move.\n");
			xrayInteractCV[index]->Signal(xrayInteractLock[index]);
			if(0 == i) fprintf(stdout, "Xray Technician waits the Patient to get onto the table.\n");
			else fprintf(stdout, "Xray Technician waits the Patient to move.\n");
			xrayInteractCV[index]->Wait(xrayInteractLock[index]);
			// Random the Xray result.
			int result = rand() % 3;
			if(0 == result) examSheet->xrayImage[i] = "nothing";
			else if(1 == result) examSheet->xrayImage[i] = "break";
			else if(2 == result) examSheet->xrayImage[i] = "compound fracture";
			fprintf(stdout, "Xray Technician takes an Xray.\n");
			xrayInteractCV[index]->Signal(xrayInteractLock[index]);
		}
		fprintf(stdout, "Xray Technician waits for the Nurse to come.\n");
		xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		fprintf(stdout, "Xray Technician gives the Examination Sheet to the Nurse.\n");
		xrayInteractCV[index]->Signal(xrayInteractLock[index]);
		fprintf(stdout, "Xray Technician waits for the Nurse and Patient to leave the Xray room.\n");
		xrayInteractCV[index]->Wait(xrayInteractLock[index]);
		
		////////////////////////////////////////////////////////
		// Leave the Xray Technician interact section in roomID.
		// End of section 2 in XrayTechnician().
		xrayInteractLock[index]->Release();
	}
}

//----------------------------------------------------------------------
// Problem2
//
//
//----------------------------------------------------------------------
void Problem2()
{
	init();
}
