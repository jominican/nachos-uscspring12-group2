/*	
 *	CSCI402: Operating Systems -- Assignment 1, Problem 2.
 *	Author: Litao Deng, Hao Chen, Anqi Wu.
 *	Description: Building a synchronized, multi-threaded system.
 *
 */

////////////////
// Header files.
#include "thread.h"
#include "synch.h"
#include "system.h"

////////////////////////////////////////
// Constants for corresponding entities. 
#define MAX_DOCTOR 3
#define MAX_NURSE 5
#define MAX_XRAY 2
#define XRAY_IMAGE 3

//----------------------------------------------------------------------
// Global data structures.
//	1: Examination Sheet.
//	2: Enumerations.
//----------------------------------------------------------------------
////////////////////////////////
// Struct for Examination Sheet.
struct ExamSheet{
	int   patientID;	// binded with the Patient.
	int   age;
	char* name;
	int   examRoomID;	// binded with the Exam Room in the first time.
	bool  xray;
	int   xrayID;	// binded with the Xray Technician.
	int   numberOfXray;
	char* xrayImage[XRAY_IMAGE];	// 'nothing', 'break', 'compound fracture'.
	bool  shot;
	float price;
	char* result;	// examination result.
	bool  goToCashier;	//	go to Cashier or not.
};

/////////////////////////////////////////////
// Enumerations for the state of each entity.
enum NurseInteract_State{
	N_BUSY, N_FREE
};
enum WaitingRoomNurse_State{
	W_BUSY, W_FREE	
};
enum WRNurse_Task{
	W_GETFORM, W_GIVEFORM
};
enum ExamRoom_State{
	E_BUSY, E_FREE, E_READY, E_FINISH	// E_READY: the Examination Room is ready for Doctor; E_FINISH: the Examination Room is ready (finish) for Nurse.
};
enum ExamRoom_Task{
	E_FIRST, E_SECOND	// the first or the second time the Patient meets with the Doctor.
};
enum Xray_State{
	X_BUSY, X_FREE, X_READY, X_FINISH
};
enum Cashier_State{
	C_BUSY, C_FREE
};

//----------------------------------------------------------------------
// Global variables.
//	1: Entity numbers from the input.
//	2: Monitor Variables.
//	3: Locks.
//	4: Condition Variables.
//	5: Examination Sheets.
//	6: Entity states.
//	7: Entity tasks.
//----------------------------------------------------------------------
/////////////////////////////////
// Input numbers for each entity.
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

/////////////////////////////////////
// Monitor Variables for each entity.
int numOfNursesPassed; // in total, how many Nurses have gone to take Patients.
int numOfPatientsPassed;	// equals to twice the number of the Patients.
int nextActionForNurse; // the state to indicate a Nurse is to pick up a Patient or not.
int indexOfSheet;	// Monitor Variable: the index of the sheet kept in WRN, initial value: 0.
int indexOfPatient;	// Monitor Variable: index Of Patient in the front of the waiting queue waiting for the Nurse, initial value: 0.
int wrnNurseIndex;	// the id of the nurse who are interacting  with the patient in waitingRoom.	
int wrnPatientID;
int patientID[MAX_NURSE];
int patientWaitingCount;	// counting how many Patients are waiting in line.
int patientWaitNurseCount; // the number of Patients waiting for the Nurse.
int nurseWaitPatientCount;	// the number of Nurses waiting for the Patient.
int nurseWaitWrnCount;	// the number of nurses waiting for the waiting room nurse.
int examRoomNurseID[MAX_NURSE];
int examRoomDoctorID[MAX_NURSE];
int examRoomPatientID[MAX_NURSE];
int examRoomSecondTimeID[MAX_XRAY];	// the ID for the Examination Room in the second visit.
int xrayRoomID[MAX_XRAY];
int xrayWaitingCount[MAX_XRAY];
int xrayPatientID[MAX_XRAY];
int cashierWaitingCount;
int cashierPatientID;
int* childToParent;	// Child to corresponding Parent mapping (need to dynamically allocate).
List* nurseTakeSheetID;

/////////////////////////
// Locks for each entity.
Lock* WRLock;	// set the lock for entering the Waiting Room.
Lock* WRInteractLock;	// set the lock for the Waiting Room Nurse and the Patient to interact.
Lock* patientWaitNurseLock;	// the lock of ready Patient waiting for the Nurse to escort.
Lock* nurseLock[MAX_NURSE];	// the lock for the interaction between Nurse and Patient.
Lock* nurseWrnLock;
Lock* examRoomLock;
Lock* examRoomLockArray[MAX_NURSE];
Lock* xrayCheckingLock;
Lock* xrayWaitingLock[MAX_XRAY];
Lock* xrayInteractLock[MAX_XRAY];	// there could be MAX_XRAY (2) Xray Technicians.
Lock* cashierWaitingLock;
Lock* cashierInteractLock;	// there is just one single Cashier.
Lock* cabinetLock;

///////////////////////////////////////
// Condition Variables for each entity.
Condition* WRInteractCV;		// set the Condition Variable for the Waiting Room Nurse and the Patient to interact.
Condition* patientWaitingCV;	// set the Condition Variable that Patients are waiting for in line.
Condition* patientWaitNurseCV;  // the Condition Variable for the Monitor.
Condition* nurseCV[MAX_NURSE];		// the Condition Variable for the interaction between Nurse and Patient.
Condition* nurseWrnCV;		// the Condition Variable for Nurses waiting for the Waiting Room Nurse.
Condition* nurseWaitPatientCV;
Condition* examRoomCV;
Condition* examRoomCVArray[MAX_NURSE];
Condition* xrayWaitingCV[MAX_XRAY];
Condition* xrayInteractCV[MAX_XRAY];
Condition* cashierWaitingCV;
Condition* cashierInteractCV;

///////////////////////////////
// Semaphores for each entity.
Semaphore* nurseWrnSemaphore;

//////////////////////////////////////
// Examination Sheets for each entity.
ExamSheet* wrnExamSheet;		// the Waiting Room Nurse makes a new exam sheet for a new Patient.
ExamSheet* nurseExamSheet[MAX_NURSE];
ExamSheet* examSheetArray[100];		// the Exam Sheet binded with different Exam Rooms.
ExamSheet* examRoomExamSheet[MAX_NURSE];	// the current Examination Sheet from the Patient in Doctor().
ExamSheet* xrayExamSheet[MAX_XRAY];	// the current Examination Sheet from the Patient in XrayTechnician().
ExamSheet* cashierExamSheet;	// the current Examination Sheet from the Patient in Cashier().

//////////////////////////
// States for each entity.
WaitingRoomNurse_State waitingRoomNurseState;
NurseInteract_State isNurseInteract;
ExamRoom_State examRoomState[MAX_NURSE];
Xray_State xrayState[MAX_XRAY];
Cashier_State cashierState;

/////////////////////////
// Tasks for each entity.
WRNurse_Task WRNurseTask;
ExamRoom_Task examRoomTask[MAX_NURSE];

// the current remaining Patient number.
int remainPatientCount;

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
	// Initialize the Waiting Room Nurse().
	wrnNurseIndex = 0;
	wrnPatientID = 0;
	indexOfSheet = 0;
	indexOfPatient = 0;
	numOfPatientsPassed = 0;
	WRLock = new Lock("WRLock");
	WRInteractLock = new Lock("WRInteractLock");
	WRInteractCV = new Condition("WRInteractCV");
	waitingRoomNurseState = W_BUSY;
	
	// Initialize the Patient().
	patientWaitingCount = 0;
	patientWaitNurseCount = 0;
	childToParent = new int[numberOfPatients];
	patientWaitNurseLock = new Lock("patientWaitNurseLock");
	patientWaitingCV = new Condition("patientWaitingCV");
	patientWaitNurseCV = new Condition("patientWaitNurseCV");
	
	// Initialize the Nurse().
	nurseWaitPatientCount = 0;
	numOfNursesPassed = 0;
	nurseWaitWrnCount = 0;
	nextActionForNurse = 0;
	nurseTakeSheetID = new List;
	for(int i = 0; i < MAX_NURSE; ++i){
		nurseLock[i] = new Lock("nurseLock");
	}
	for(int i = 0; i < MAX_NURSE; ++i){
		nurseCV[i] = new Condition("nurseCV");
	}
	nurseWrnLock = new Lock("nurseWrnLock");
	nurseWrnCV = new Condition("nurseWrnCV");
	nurseWaitPatientCV = new Condition("nurseWaitPatientCV");
	nurseWrnSemaphore = new Semaphore("nurseWrnSemaphore", 0);
	isNurseInteract = N_FREE;
	
	// Initialize the Doctor().
	examRoomLock = new Lock("examRoomLock");
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
	cashierState = C_BUSY;
	
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
		xrayState[i] = X_BUSY;
	}
	
	cabinetLock = new Lock("cabinetLock");
	
	// Initialize the Parent().
}

//----------------------------------------------------------------------
// Doctor
//
//
//----------------------------------------------------------------------
void Doctor(int index)
{
	while(remainPatientCount){
		// Check for the state of the Examination Rooms.
		//examRoomCheckingLock->Acquire();
		examRoomLock->Acquire();
		int roomIndex = -1;
		for(int i = 0; i < numberOfNurses; ++i){
			if(E_READY == examRoomState[i]){
				examRoomState[i] = E_BUSY;
				roomIndex = i; break;
			}
		}
		//examRoomCheckingLock->Release();
		examRoomLock->Release();
		
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
				result = 0;
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
				examRoomCVArray[roomIndex]->Signal(examRoomLockArray[roomIndex]);
			}else if(E_SECOND == examRoomTask[roomIndex]){
				//examRoomCVArray[roomIndex]->Broadcast(examRoomLockArray[roomIndex]);
				//examRoomCVArray[roomIndex]->Wait(examRoomLockArray[roomIndex]);
				fprintf(stdout, "Doctor [%d] is examining the Xrays of [Adult/Child] Patient [%d].\n", index, examRoomPatientID[roomIndex]);
				fprintf(stdout, "Doctor [%d] has left the examination room.\n", index);
				examRoomCVArray[roomIndex]->Signal(examRoomLockArray[roomIndex]);
				examRoomCVArray[roomIndex]->Wait(examRoomLockArray[roomIndex]);
				fprintf(stdout, "Doctor finishes the second time.\n");
			}
			// Loop into the next iteration.
			examRoomLockArray[roomIndex]->Release();
		}else{	// No room is ready for the Doctor.
			// Avoid busy waiting.
			int r = rand() % 100;
			for(int i = 0; i < r; ++i){
				currentThread->Yield();
			}
		}
	}
}

//----------------------------------------------------------------------
// Nurse
//
//
//----------------------------------------------------------------------
void Nurse(int index)
{
    ExamSheet* patientExamSheet;
    while(remainPatientCount)
	{
		fprintf(stdout, "Nurse.\n");
		////////////////////////////////////////////////////////
		// Task 1: escort the Patients from Waiting Room to the Exam Room.
		examRoomLock->Acquire();	// go to see if there is any Doctor/Examination Room available.
		int i = 0;
	    for(; i < numberOfNurses; ++i)	// check to see which Examination Room is avaiable.
            if(examRoomState[i] == E_FREE)
            {
			    break;
			}
        if(i == numberOfNurses)	// no Examination Room is availble at this time.
		{
		    //fprintf(stdout, "Nurse %d doesn't find any available examinationRoom, so, he/she releases the allExamRoomLock.\n");
		    examRoomLock->Release();
		}
		else
		{		    
			examRoomState[i] = E_BUSY;
			//fprintf(stdout, "Nurse %d tries to get the lock of examRoom %d.\n", index,i);
			examRoomLock->Release();
			nurseWrnLock->Acquire();
			nurseTakeSheetID->Append((void *)&index);	// ???
			nurseWaitWrnCount++;
			nurseWrnCV->Wait(nurseWrnLock);
			nurseWaitWrnCount--;
			if(nextActionForNurse == 1)
			{
				patientExamSheet = wrnExamSheet;				
				nurseWrnLock->Release();
				
				patientWaitNurseLock->Acquire();
				nurseWaitPatientCount++;
				if(isNurseInteract == N_BUSY)
				{
					nurseWaitPatientCV->Wait(patientWaitNurseLock);
				}
				isNurseInteract = N_BUSY;
				nurseWaitPatientCount--;
				patientWaitNurseCount--;
				wrnNurseIndex = index; 	// the Nurse would escort the Patient.
				patientWaitNurseCV->Signal(patientWaitNurseLock);
			
				nurseLock[index]->Acquire();
				patientWaitNurseLock->Release();
				nurseWrnSemaphore->V();
				nurseCV[index]->Wait(nurseLock[index]);
				fprintf(stdout, "Nurse [%d] escorts Adult Patient /Parent [%d] to the waiting room.\n", index, patientID[index]);
			
				int patient_ID = patientID[index];				
				nurseExamSheet[index]->examRoomID = i;	// assign the Exam Room number.
				nurseCV[index]->Signal(nurseLock[index]);
				examRoomLockArray[i]->Acquire();
				examRoomTask[i] = E_FIRST;
                
				fprintf(stdout, "Nurse [%d] takes the temperature and blood pressure of Adult Patient /Parent [%d].\n", index, patientID[index]);
				fprintf(stdout, "Nurse [%d] asks Adult Patient /Parent [%d] What Symptoms do you have?\n", index, patientID[index]);
				fprintf(stdout, "Nurse [%d] writes all the information of Adult Patient /Parent [%d] in his examination sheet.\n", index, patientID[index]);
			    nurseLock[index]->Release();
			    examRoomCVArray[i]->Wait(examRoomLockArray[i]);	
				fprintf(stdout, "Nurse [%d] informs Doctor [%d] that Adult/Child Patient [%d] is waiting in the examination room [%d].\n", index,examRoomDoctorID[i],patient_ID,i);
				fprintf(stdout, "Nurse [%d] hands over to the Doctor [%d] the examination sheet of Adult/Child Patient [%d].\n", index,examRoomDoctorID[i],patient_ID);
				examRoomLockArray[i]->Release();			
			}
		    else
			{
			    examRoomLock->Acquire();
				examRoomState[i] = E_FREE;
				examRoomLock->Release();
				nurseWrnLock->Release();
			}
		}
		// end of Task 1.

		// Avoid busy waiting.
		srand(time(0));
		int r = rand() % 100 + 50;
		int loopTime = 0;
		for(; loopTime != r; ++loopTime)
			currentThread->Yield();
			
		///////////////////////////////////////////////////	
		// Task 2: escort the Patients from Exam Room to Xray Room.
		examRoomLock->Acquire();	// to go to get a Patient.
		int loopRoomID = 0;	// the id for examRoomID.
		for(; loopRoomID < numberOfNurses; ++loopRoomID)
		{
			if(examRoomState[loopRoomID] == E_FINISH)
			{
				examRoomState[loopRoomID] = E_BUSY;
				break;
			}
		}
		examRoomLock->Release();	 

		if(loopRoomID != numberOfNurses)	// when there is Patient with the state of FINISH.
		{
			examRoomLockArray[loopRoomID]->Acquire();
			patientExamSheet = examRoomExamSheet[loopRoomID];
			if(patientExamSheet->xray && !(patientExamSheet->goToCashier))
			{
				examRoomNurseID[loopRoomID] = index;	
				examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);	// signal a person in the Examination Room.
				patientExamSheet = examRoomExamSheet[loopRoomID];
				int xrayLoop = 0;
				int index_xray;
				xrayCheckingLock->Acquire();	// check the state of the Xray Room.
				for(; xrayLoop != numberOfXrays; ++xrayLoop)
				{
					if(xrayState[xrayLoop] == X_FREE)
					{
						xrayState[xrayLoop] = X_BUSY;
						break;
					}
				}
				xrayCheckingLock->Release();
				if(xrayLoop != numberOfXrays)	// there is FREE Xray Room.
				{
					xrayRoomID[loopRoomID] = xrayLoop;
				}else{
					srand(time(0));
					index_xray = rand() % 2;
					xrayRoomID[loopRoomID] = index_xray;
				}
			}
			else if(patientExamSheet->shot && !(patientExamSheet->goToCashier))
			{
				examRoomNurseID[loopRoomID] = index;	
				examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);	// signal a person in the Examination Room.
				patientExamSheet = examRoomExamSheet[loopRoomID];
				fprintf(stdout, "Nurse [%d] goes to supply cabinet to give to take medicine for Adult Patient /Parent [%d].\n", index, patientExamSheet->patientID);
				cabinetLock->Acquire();
				// do sth in Cabinet.
				cabinetLock->Release();
				fprintf(stdout, "Nurse [%d] asks Adult Patient /Parent [%d] Whether you are ready for the shot?\n", index, patientExamSheet->patientID);
				examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);
				examRoomCVArray[loopRoomID]->Wait(examRoomLockArray[loopRoomID]);
				fprintf(stdout, "Nurse [%d] tells Adult Patient /Parent [%d] Shot is over.\n", index, patientExamSheet->patientID);
				fprintf(stdout, "Nurse [%d] escorts Adult Patient /Parent [%d] to Cashier.\n", index, patientExamSheet->patientID);
				examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);			
			}else{
				examRoomLock->Acquire();
				examRoomState[loopRoomID] = E_FINISH;	// ???
				examRoomLock->Release();				
			}
			examRoomLockArray[loopRoomID]->Release();
		}	
		else{	// no Patient is in state of FINISH.
		}		

		srand(time(0));
		r = rand() % 100 + 50;
		for(loopTime = 0; loopTime != r; ++loopTime)
			currentThread->Yield();

		/////////////////////////////////////////
		// Task 3: escort from Xray Room to Exam Room.
		examRoomLock->Acquire();
		int loopID = 0; //	to search a free room in the examRoomLock.
		for(; loopID < numberOfNurses; ++loopID)
		{
			if(examRoomState[loopID] == E_FREE)
			{
				examRoomState[loopID] = E_BUSY;
				break;
			}	
		}
		examRoomLock->Release();

		if(loopID != numberOfNurses) // there is a FREE Exam Room.
		{
			xrayCheckingLock->Acquire();
			int tempXrayID = 0;
			for(; tempXrayID < numberOfXrays; ++tempXrayID)
			{
				if(xrayState[tempXrayID] == X_FINISH)
				{
					xrayState[tempXrayID] = X_BUSY;	// ???
					break;
				}
			}
			xrayCheckingLock->Release();
			if(tempXrayID != numberOfXrays)	// one patient finishs in a xrayRoom.
			{
				xrayInteractLock[tempXrayID]->Acquire();
				examRoomSecondTimeID[tempXrayID] = loopID;
				xrayInteractCV[tempXrayID]->Broadcast(xrayInteractLock[tempXrayID]);
				examRoomLockArray[loopID]->Acquire();
				xrayInteractLock[tempXrayID]->Release();
				examRoomCVArray[loopID]->Wait(examRoomLockArray[loopID]);
				fprintf(stdout, "Nurse [%d] informs Doctor [%d] that Adult/Child Patient [%d] is waiting in the examination room %d.\n", index,examRoomDoctorID[loopID],patientID[index],loopID); //������
				fprintf(stdout, "Nurse [%d] hands over to the Doctor [%d]  the examination sheet of Adult/Child Patient [%d].\n", index,examRoomDoctorID[loopID],patientID[index]);//������
				examRoomLockArray[loopID]->Release();
			}else{
				examRoomLock->Acquire();
				examRoomState[loopID] = E_FREE;
				examRoomLock->Release();
			}
		}
        // end of Task 3.		
		
		srand(time(0));
		r = rand() % 100 + 50;
		for(loopTime = 0; loopTime != r; ++loopTime)
			currentThread->Yield();
		
		////////////////////////////////////////////////////////////
		// Task 4: go to make sure that the Patient is go to Cahier.
		examRoomLock->Acquire();
		loopID = 0;	// to search a free room in the examRoomLock.
		for(; loopID < numberOfNurses; ++loopID)
		{
			if(examRoomState[loopID] == E_FINISH)
			{
				examRoomState[loopID] = E_BUSY;
				break;
			}	
		}
		examRoomLock->Release();

		if(loopID != numberOfNurses)
		{
			examRoomLockArray[loopID]->Acquire();
			if(examRoomExamSheet[loopID]->goToCashier)
			{
				fprintf(stdout, "Nurse [%d] escorts Adult Patient /Parent [%d] to Cashier.\n", index, examRoomExamSheet[loopID]->patientID);
				examRoomCVArray[loopID]->Signal(examRoomLockArray[loopID]);
				fprintf(stdout, "the Signal is [%d].\n", loopID);
			}
			else
			{
				examRoomLock->Acquire();
				examRoomState[loopID] = E_FINISH;
				examRoomLock->Release();				
			}
			fprintf(stdout, "!!!!!!Room ID [%d].\n", loopID);
			examRoomLockArray[loopID]->Release();
		}
		srand(time(0));
		r = rand() % 100 + 50;
		for(loopTime = 0; loopTime != r; ++loopTime)
			currentThread->Yield();   		
	}
}

//----------------------------------------------------------------------
// WaitingRoomNurse: Anqi Wu
//	The Waiting Room Nurse stays at her desk in the Waiting Room. 
//	Their job is to give a form to Patients to fill out. The form requires the patient name and age. 
//	The Waiting Room Nurse does not wait for a Patient to fill out the form. 
//	Once the Waiting Room Nurse has accepted the completed form, they tell the Patient to go back and wait to be called by a Nurse.
//	The Waiting Room Nurse create a new Examination Sheet for a registered Patient. 
//	They write the Patient's name and age on the Examination Sheet. 
//	The Waiting Room Nurse holds onto all examination sheets for Patients that are in the waiting room. 
//	When a Nurse is to escort a Patient to an examination room, they receive the appropriate examination sheet from the Waiting Room Nurse.
//	Nurses will tell the Waiting Room Nurse when there is an open examinination room. 
//	The Waiting Room Nurse will tell the Nurse if there is a Patient waiting, or not. 
//----------------------------------------------------------------------
void WaitingRoomNurse(int index) {
	while (remainPatientCount) {
		fprintf(stdout, "Waiting Room Nurse [%d] [%d].\n", patientWaitNurseCount, nurseWaitWrnCount);
		if(numOfPatientsPassed < numberOfPatients){
			WRLock->Acquire();
			if (patientWaitingCount > 0) {		// a Patient is in line.
				patientWaitingCV->Signal(WRLock);		// the Waiting Room Nurse wake a Patient up.
				patientWaitingCount--;
				waitingRoomNurseState = W_BUSY;		// make the Waiting Room Nurse busy when she wakes up a Patient.
			}else {
				waitingRoomNurseState = W_FREE;		// if no one is in line, the Waiting Room Nurse is free.
			}
		
			WRInteractLock->Acquire();		// the Waiting Room Nurse gets ready for talking with Patient first.
			WRLock->Release();		// let the Patient have the right to act.
			WRInteractCV->Wait(WRInteractLock);		// wait for the Patient coming to tell the task.
			// see what task to perform.
			
			if (WRNurseTask == W_GETFORM) {
				wrnExamSheet = new ExamSheet();		// make a new exam sheet.
				wrnExamSheet->xray = false;
				wrnExamSheet->shot = false;
				wrnExamSheet->goToCashier = false;
				fprintf(stdout, "Waiting Room nurse gives a form to Adult patient [%d].\n", wrnPatientID);
				WRInteractCV->Signal(WRInteractLock);		// tell the Patient that she makes a new sheet.
				WRInteractCV->Wait(WRInteractLock);		// wait for Patient to tell her they get the sheet.
			}		// done with a Patient's getting form task.
			else if (WRNurseTask == W_GIVEFORM) {
				fprintf(stdout, "Waiting Room nurse accepts the form from Adult Patient/Parent with name [%s] and age [%d].\n", wrnExamSheet->name, wrnExamSheet->age);
				examSheetArray[indexOfSheet++] = wrnExamSheet;	// dynamically allocation???
				fprintf(stdout, "Waiting Room nurse creates an examination sheet for [Adult/Child] patient with name [%s] and age [%d].\n", wrnExamSheet->name, wrnExamSheet->age);
				fprintf(stdout, "Waiting Room nurse tells the Adult Patient/Parent [%d] to wait in the waiting room for a nurse.\n", wrnExamSheet->patientID);
			}
			WRInteractLock->Release();		
		}
		
		// Waiting Room Nurse tries to get the nurseWrnLock to give an Exam Sheet to the Nurse.
		nurseWrnLock->Acquire();
		if(nurseWaitWrnCount > 0) {
			// there's Nurse waiting for escorting Patient.
			if(indexOfSheet > numOfNursesPassed)
			{
				// there are more registered Patients than Nurses.
				wrnExamSheet = examSheetArray[indexOfPatient++];
				int nurseTakeSheet_ID = *((int *)nurseTakeSheetID->Remove());
				numOfNursesPassed++;
				nextActionForNurse = 1; // tell Nurse to go to pick up a Patient.
				fprintf(stdout, "Waiting Room nurse gives examination sheet of patient [%d] to Nurse [%d].\n", wrnExamSheet->patientID, nurseTakeSheet_ID);
				nurseWrnCV->Signal(nurseWrnLock);
				// WRN tries to release nurseWrnLock after giving the Exam Sheet to a Nurse.
				nurseWrnLock->Release();		
				nurseWrnSemaphore->P();
			}
			else
			{
				// WRN finds that there is no Patient waiting for the Nurse or there is enough Nurses for the Patient.
				int nurseTakeSheet_ID = *((int *)nurseTakeSheetID->Remove());
				nextActionForNurse = 0; // not go to pick Patient.
				nurseWrnCV->Signal(nurseWrnLock);
				// WRN tries to release nurseWrnLock because there is no Patient waiting.
				nurseWrnLock->Release();
			}
		}
		else
		{	// no Nurse waits in line.
			// WRN tries to release nurseWrnLock because there is no Nurse waiting.
			nurseWrnLock->Release();
		}
		
		int r  = 100;
		for(int i = 0 ; i < r; ++i)
			currentThread->Yield();
		//WRInteractLock->Release();		
	}	//end of while
}

//----------------------------------------------------------------------
// Cashier
//
//
//----------------------------------------------------------------------
void Cashier(int index)
{
	while(remainPatientCount){
		fprintf(stdout, "Cashier.\n");
		if(cashierWaitingCount > 0){
			////////////////////////////////////////////
			// Try to enter the Cashier waiting section.
			// There may be some Patients waiting for the Cashier.
			// Start of section 1 in Cashier().
			cashierWaitingLock->Acquire();
			if(cashierWaitingCount > 0){
				// Some Patients are in line.
				// Wake them up.
				cashierWaitingCV->Signal(cashierWaitingLock);
				cashierWaitingCount--;
				cashierState = C_BUSY;
			}else{
				// No patients are in line.
				cashierState = C_FREE;
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
			
			if(remainPatientCount)	// there would be one Patient to wake up the Cashier.
				cashierInteractCV->Wait(cashierInteractLock);
			else break;	// all of the Patient has left the Doctor's office.
			fprintf(stdout, "Cashier receives the examination sheet from Adult Patient [%d].\n", cashierPatientID);
			fprintf(stdout, "Cashier reads the examination sheet of Adult Patient [%d] and asks him to pay $402\n", cashierPatientID);
			cashierInteractCV->Signal(cashierInteractLock);
			cashierInteractCV->Wait(cashierInteractLock);
			fprintf(stdout, "Cashier accepts $402 from Adult Patient [%d].\n",  cashierPatientID);
			fprintf(stdout, "Cashier gives a receipt of $402 to Adult Patient [%d].\n", cashierPatientID);
			cashierInteractCV->Signal(cashierInteractLock);
			cashierInteractCV->Wait(cashierInteractLock);
			fprintf(stdout, "Cashier waits for the Patient to leave.\n", cashierPatientID);
			cashierInteractCV->Signal(cashierInteractLock);
			cashierInteractCV->Wait(cashierInteractLock);
			
			//////////////////////////////////////
			// Leave the Cashier interact section.
			// End of section 2 in Cashier().
			cashierInteractLock->Release();
		}else{
			int r = 100;
			for(int i = 0; i < r; ++i)
				currentThread->Yield();
		}
	}
}

//----------------------------------------------------------------------
// XrayTechnician
//
//
//----------------------------------------------------------------------
void XrayTechnician(int index)
{
	while(remainPatientCount){
		fprintf(stdout, "Xray Technician.\n");
		/////////////////////////////////////////
		// Try to enter the Xray waiting section.
		// Start of section 1 in XrayTechnician().
		if(xrayWaitingCount[index] > 0){
			xrayWaitingLock[index]->Acquire();
			// There is one room waiting for Xray Technician.
			if(xrayWaitingCount[index] > 0){
				xrayWaitingCV[index]->Signal(xrayWaitingLock[index]);
				fprintf(stdout, "XrayTechnician [%d] has waked up the CV.\n", index);
				xrayWaitingCount[index]--;
				xrayState[index] = X_BUSY;
			}else{
				fprintf(stdout, "XrayTechnician [%d] is FREE.\n", index);
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
			for(int i = 0; i < xrayExamSheet[index]->numberOfXray; ++i){
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
		}else{
			int r = 100;
			for(int i = 0; i < 100; ++i)
				currentThread->Yield();
		}
	}
}

//----------------------------------------------------------------------
// Patient
//
//
//----------------------------------------------------------------------
void Patient(int index) {
	
	ExamSheet* myExamSheet;		// the Patient declares a new Exam Sheet for himself.
	char patient_id[4];
	char patient_name[13];
	int patient_ID = index;
	
	fprintf(stdout, "Adult Patient [%d] has entered the Doctor's Office.\n", patient_ID);
	// the Patient gets in the line for the first time to get a form.
	WRLock->Acquire();		// the Patient tries to enter the Waiting Room and stay in line.
	fprintf(stdout, "Adult Patient [%d] gets in line of the Waiting Room Nurse to get registration form.\n", patient_ID);
	if (waitingRoomNurseState == W_BUSY) {		// the Waiting Room Nurse is busy. then the Patient gets in line.
		patientWaitingCount++;
		patientWaitingCV->Wait(WRLock);
	}else {
		waitingRoomNurseState = W_BUSY;
	}
	WRLock->Release();		// the Patient finishes waiting and gets ready to interact with the Waiting Room Nurse.
	
	WRInteractLock->Acquire();
	//need to tell the Waiting Room Nurse what to do -- firstly get a form.
	WRNurseTask = W_GETFORM;
	wrnPatientID = patient_ID;
	WRInteractCV->Signal(WRInteractLock);		// tell the Waiting Room Nurse that he comes.
	WRInteractCV->Wait(WRInteractLock);		// wait for the Waiting Room Nurse to make a new sheet.
	fprintf(stdout, "[Adult patient/ Parent of child patient] get the form from the Waiting Room Nurse.\n");
	myExamSheet = wrnExamSheet;		// get the Exam Sheet.
	WRInteractCV->Signal(WRInteractLock);		// tell the Waiting Room Nurse that he gets the sheet.
	WRInteractLock->Release();
	
	// Patient goes away and fills the form.
	myExamSheet->patientID = patient_ID;		// write the Patient id, his name, on the sheet.
	srand(time(NULL));
	myExamSheet->age = rand() % 50 + 15;		// write the age of the Patient on the Exam Sheet. Adult Patient age should be larger than 15.
	sprintf(patient_id, "%d ", patient_ID);
	strcpy(patient_name, "Patient_");
	strcat(patient_name, patient_id);
	myExamSheet->name = patient_name;		// write the name string on the Exam Sheet struct for print.
	
	// the Patient gets back to the line to submit the form.
	WRLock->Acquire();		// the Patient tries to enter the Waiting Room and stay in line.
	fprintf(stdout, "Adult Patient [%d] gets in line of the Waiting Room Nurse to submit registration form.\n", patient_ID);
	if (waitingRoomNurseState == W_BUSY) {		// the Waiting Room Nurse is busy. then the Patient gets in line.
		patientWaitingCount++;
		patientWaitingCV->Wait(WRLock);
	}else {
		waitingRoomNurseState = W_BUSY;
	}
	WRLock->Release();		// the Patient finishes waiting and gets ready to interact with the Waiting Room Nurse.
	
	WRInteractLock->Acquire();
	// need to tell the Waiting Room Nurse what to do -- secondly turn over a form.
	WRNurseTask = W_GIVEFORM;
	fprintf(stdout, "[Adult patient/ Parent of child patient] submit the filled form to the Waiting Room Nurse.\n");
	wrnExamSheet = myExamSheet;
	numOfPatientsPassed++;	// one Patient passed the interaction with the Waiting Room Nurse.
	WRInteractCV->Signal(WRInteractLock);		// turn over the filled form.
	
	// Patient waits for a Nurse in line.
	patientWaitNurseLock->Acquire();
	patientWaitNurseCount++;
	WRInteractLock->Release();		// Patient leaves the Waiting Room Nurse.
	
	patientWaitNurseCV->Wait(patientWaitNurseLock);		// Patient waits for a Nurse come to escort him.
	if(nurseWaitPatientCount > 0)
	{	// before leaving, wake up the next Nurse in line to get prepared for escorting the next Patient in line away.
		nurseWaitPatientCV->Signal(patientWaitNurseLock);
		isNurseInteract = N_BUSY;
	}
	else
	{
		isNurseInteract = N_FREE;
	}
	
	// escorted away by the Nurse.
	int nurse_id = wrnNurseIndex;		// Patient knows which Nurse escorts him/her away.
	patientWaitNurseLock->Release();
	
	// Patient tries to acquire the lock for Nurse.
	nurseLock[nurse_id]->Acquire();
	patientID[nurse_id] = patient_ID;
	nurseExamSheet[nurse_id] = myExamSheet;
	nurseCV[nurse_id]->Signal(nurseLock[nurse_id]);
	nurseCV[nurse_id]->Wait(nurseLock[nurse_id]);		// waiting for the Nurse to get his Exam Sheet.
	myExamSheet = nurseExamSheet[nurse_id];		// get information back from the Nurse and know symptoms.
	nurseLock[nurse_id]->Release();
	
	// Patient enters the Exam Room.
	int examRoom_id = myExamSheet->examRoomID;
	examRoomLockArray[examRoom_id]->Acquire();
	examRoomLock->Acquire();
	examRoomState[examRoom_id] = E_READY;
	examRoomTask[examRoom_id] = E_FIRST;
	examRoomLock->Release();
	examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);		// get into a Exam Room and wait for a Doctor, together with a Nurse.
	
	fprintf(stdout, "Adult Patient/Parent [%d] says, �My/His symptoms are Pain/Nausea/Hear Alien Voices�.\n", patient_ID);
	examRoomExamSheet[examRoom_id] = myExamSheet;		// turn over the exam sheet to the Doctor.
	examRoomCVArray[examRoom_id]->Signal(examRoomLockArray[examRoom_id]);		
	examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);		
	myExamSheet = examRoomExamSheet[examRoom_id];		// Doctor finishes the first time Examination.
	
	if (myExamSheet->xray) {		// the Patient needs to take an Xray.
		fprintf(stdout, "Adult Patient [%d] has been informed by Doctor [%d] that he needs an Xray.\n", patient_ID, examRoomDoctorID[examRoom_id]);
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FINISH;
		examRoomLock->Release();
		fprintf(stdout, "Adult Patient [%d] waits for a Nurse to escort them to Xray room.\n", patient_ID);
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);	// the Patient waits for a Nurse to escort him/her to Xay Room.
		
		// a Nurse comes to escort Patient away.
		int examRoomNurse_id = examRoomNurseID[examRoom_id];		// record the Nurse who takes Patient away from the Exam Room for the first time.
		int xrayRoom_id = xrayRoomID[examRoom_id];		// the Xray Room he needs to go to.
		examRoomLockArray[examRoom_id]->Release();	
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FREE;
		examRoomLock->Release();
		
		xrayWaitingLock[xrayRoom_id]->Acquire();		
		if (xrayState[xrayRoom_id] == X_BUSY || xrayState[xrayRoom_id] == X_FINISH) {		// the Xray Technician is BUSY or FINISH. then the Patient gets in line.
			xrayWaitingCount[xrayRoom_id]++;
			xrayWaitingCV[xrayRoom_id]->Wait(xrayWaitingLock[xrayRoom_id]);
		}else {
			xrayState[xrayRoom_id] == X_BUSY;
		}
		xrayWaitingLock[xrayRoom_id]->Release();	
		
		// the Patient enters the Xray Room.
		xrayInteractLock[xrayRoom_id]->Acquire();
		fprintf(stdout, "Nurse [%d] informs X-Ray Technician [%d] about Adult/Child Patient [%d] and hands over the examination sheet.\n", examRoomNurse_id, xrayRoom_id, patient_ID);
		fprintf(stdout, "Nurse [%d] leaves the X-ray Room [%d].\n", examRoomNurse_id, xrayRoom_id);
		xrayExamSheet[xrayRoom_id] = myExamSheet;
		xrayPatientID[xrayRoom_id] = patient_ID;
		xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);// the Patient gives the Exam Sheet to the Xray Technician.
		
		// Start to take xrays.
		int i = 0;
		while (i < myExamSheet->numberOfXray) {
			xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);
			if (i == 0) {
				fprintf(stdout, "Adult Patient [%d] gets on the table.\n", patient_ID);
			}
			xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);
			xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);
			fprintf(stdout, "Adult Patient [%d] has been asked to take an Xray.\n", patient_ID);
			xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);
			i++;
		}		// Xray Technician finishes taking xrays for the Patient.
		
		xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);
		fprintf(stdout, "Adult Patient [%d] waits for a Nurse to escort him/her to exam room.\n", patient_ID);
		xrayCheckingLock->Acquire();
		xrayState[xrayRoom_id] = X_FINISH;
		xrayCheckingLock->Release();
		xrayInteractCV[xrayRoom_id]->Wait(xrayInteractLock[xrayRoom_id]);	// the Patient is waiting for a Nurse to take him/her back to the Exam Room.

		//	a second Nurse comes to escort Patient away back to Exam Room.
		int examRoomSecondTime_id = examRoomSecondTimeID[xrayRoom_id];
		xrayInteractLock[xrayRoom_id]->Release();
		examRoomLockArray[examRoomSecondTime_id]->Acquire();
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		examRoomLock->Acquire();
		examRoomState[examRoomSecondTime_id] = E_READY;
		examRoomTask[examRoomSecondTime_id] = E_SECOND;
		examRoomLock->Release();
		examRoomCVArray[examRoomSecondTime_id]->Wait(examRoomLockArray[examRoomSecondTime_id]);	// get into a Exam Room and wait for a Doctor, together with a Nurse.
		
		examRoomCVArray[examRoomSecondTime_id]->Signal(examRoomLockArray[examRoomSecondTime_id]);	
		examRoomCVArray[examRoomSecondTime_id]->Wait(examRoomLockArray[examRoomSecondTime_id]);	
		fprintf(stdout, "Adult Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, examRoomDoctorID[examRoomSecondTime_id]);
		examRoomCVArray[examRoomSecondTime_id]->Signal(examRoomLockArray[examRoomSecondTime_id]);	
		examRoomCVArray[examRoomSecondTime_id]->Wait(examRoomLockArray[examRoomSecondTime_id]);	
		examRoomLock->Acquire();
		examRoomState[examRoomSecondTime_id] = E_FINISH;
		myExamSheet->goToCashier = true;
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		examRoomLock->Release();
		// the Patient waits for a third Nurse to take him/her to the Cashier.
		fprintf(stdout, "!!!!!!!!>>>>>>>>>>>>\n");
		examRoomCVArray[examRoomSecondTime_id]->Wait(examRoomLockArray[examRoomSecondTime_id]);
		fprintf(stdout, ">>>>>>>>>>>>!!!!!!!!\n");
		examRoomLockArray[examRoomSecondTime_id]->Release();	
		examRoomLock->Acquire();
		examRoomState[examRoomSecondTime_id] = E_FREE;
		examRoomLock->Release();
	}
	else if (myExamSheet->shot) {	// the Patient needs a shot.
		fprintf(stdout, "Adult Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, examRoomDoctorID[examRoom_id]);
		fprintf(stdout, "Adult Patient [%d] has been informed by Doctor [%d] that he will be administered a shot.\n", patient_ID, examRoomDoctorID[examRoom_id]);
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FINISH;
		examRoomLock->Release();
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);
		
		fprintf(stdout, "Adult Patient/Parent [%d] says, �Yes I am/He is ready for the shot�.\n", patient_ID);
		examRoomCVArray[examRoom_id]->Signal(examRoomLockArray[examRoom_id]);
		// the Patient waits for the Nurse to take him/her to the Cashier.
		myExamSheet->goToCashier = false;
		examRoomExamSheet[examRoom_id] = myExamSheet;
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);
		examRoomLockArray[examRoom_id]->Release();
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FREE;
		examRoomLock->Release();
	}
	else {	// the Patient has no problem and can go to the Cashier directly.
		fprintf(stdout, "Adult Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, examRoomDoctorID[examRoom_id]);
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FINISH;
		myExamSheet->goToCashier = true;
		examRoomExamSheet[examRoom_id] = myExamSheet;
		examRoomLock->Release();
		// the Patient waits for a second Nurse to take him/her to the Cashier.
		examRoomCVArray[examRoom_id]->Wait(examRoomLockArray[examRoom_id]);
		examRoomLockArray[examRoom_id]->Release();
		examRoomLock->Acquire();
		examRoomState[examRoom_id] = E_FREE;
		examRoomLock->Release();
	}

	fprintf(stdout, "Patient [%d] has left the Doctor's office.\n", patient_ID);
	// Patient is escorted to the Cashier.
	cashierWaitingLock->Acquire();		
	if (cashierState == C_BUSY) {		
		cashierWaitingCount++;
		fprintf(stdout, "Adult Patient [%d] enters the queue for Cashier.\n", patient_ID);
		cashierWaitingCV->Wait(cashierWaitingLock);
	}else {
		cashierState = C_BUSY;
	}
	cashierWaitingLock->Release();
	
	fprintf(stdout, "Patient [%d] has entered to the Cashier.\n", patient_ID);
	// interact with the Cashier.
	cashierInteractLock->Acquire();
	fprintf(stdout, "Adult Patient [%d] reaches the Cashier.\n", patient_ID);
	fprintf(stdout, "Adult Patient [%d] hands over his examination sheet to the Cashier.\n", patient_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractCV->Wait(cashierInteractLock);	
	fprintf(stdout, "Adult Patient [%d] pays the Cashier $402.\n", patient_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractCV->Wait(cashierInteractLock);
	fprintf(stdout, "Adult Patient [%d] receives a receipt from the Cashier.\n", patient_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractCV->Wait(cashierInteractLock);
	
	// Patient finishes everything and leaves.
	fprintf(stdout, "Adult Patient [%d] leaves the doctor's office.\n", patient_ID);
	cashierInteractCV->Signal(cashierInteractLock);			
	cashierInteractLock->Release();
	if(0 == --remainPatientCount) abort();	// exit the Problem2().???
}

//----------------------------------------------------------------------
// Test1
//	Test goal: Child Patients are never abandoned by their Parent, 
//	nor go anywhere without their Parent.
//----------------------------------------------------------------------
void Test1(void)
{
	return;
}

//----------------------------------------------------------------------
// Test2
//	Test goal: Waiting Room Nurses only talk to 
//	one Patient/Parent at a time.
//----------------------------------------------------------------------
void Test2(void)
{
	return;
}

//----------------------------------------------------------------------
// Test3
//	Test goal: Cashiers only talk to one Patient/Parent at a time.
//
//----------------------------------------------------------------------
void Test3(void)
{
	return;
}

//----------------------------------------------------------------------
// Test4
//	Test goal: Patients/Parents never go anywhere 
//	without being escorted by a Nurse.
//----------------------------------------------------------------------
void Test4(void)
{
	return;
}

//----------------------------------------------------------------------
// Test5
//	Test goal: All Patients leave the Doctor's Office. No one stays in 
//	the Waiting Room, Examination Room, or Xray Room, forever.
//----------------------------------------------------------------------
void Test5(void)
{
	return;
}

//----------------------------------------------------------------------
// Test6
//	Test goal: Two Doctors never examine the same Patient 
//	at the same time.
//----------------------------------------------------------------------
void Test6(void)
{
	return;
}

//----------------------------------------------------------------------
// Problem2
//	The entrance of the Assignment 1 Problem 2.
//
//----------------------------------------------------------------------
void Problem2(void)
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
	numberOfWRNs = numberOfCashiers = 1;
	//numberOfAdults = numberOfPatients / 2; // make the numbers of Adult Patients and Child Patients as equal as possible.
	//numberOfChildren = numberOfParents = numberOfPatients - numberOfAdults;
	numberOfAdults = numberOfPatients;
	//remainPatientCount  = numberOfPatients;
	remainPatientCount = numberOfAdults;
	
	// Check the validation of the input.
	if(numberOfDoctors != 2 && numberOfDoctors != 3){
		fprintf(stderr, "The interval of the number of the Doctors is [2, 3].\n");
		return;
	}
	if(numberOfNurses < 2 || numberOfNurses > 5){
		fprintf(stderr, "The interval of the number of the Nurses is [2, 5].\n");
		return;
	}
	if(numberOfXrays != 1 && numberOfXrays != 2){
		fprintf(stderr, "The interval of the number of the Xray Technicians is [1, 2].\n");
		return;
	}
	/*
	if(numberOfPatients < 30){
		fprintf(stderr, "The interval of the number of the Patients is [30, MAX_INTEGER).\n");
		return;
	}
	*/
	
	// Fork Doctors.
	for(int i = 0; i < numberOfDoctors; ++i){
		Thread* t = new Thread("Doctor");
		t->Fork(Doctor, i);
	}
	
	// Fork Nurses.
	for(int i = 0; i < numberOfNurses; ++i){
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
	/*
	for(int i = 0; i < numberOfChildren; ++i){
		Thread* t = new Thread("Child");
		t->Fork(Child, i);
	}
	*/
	
	// Fork Parents.
	/*
	for(int i = 0; i < numberOfParents; ++i){
		Thread* t = new Thread("Parent");
		t->Fork(Parent, i);
	}
	*/
	
	//////////////////
	// Wait to finish.
	return;
}