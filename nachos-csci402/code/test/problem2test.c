/*	
 *	CSCI402: Operating Systems -- Assignment 1, Problem 2.
 *	Author: Litao Deng, Hao Chen, Anqi Wu.
 *	Description: Building a synchronized, multi-threaded system.
 */

/* Header files. */
#include "syscall.h"
#include "print.h"

/* Constants for corresponding entities. */
#define MAX_PATIENT 100
#define MAX_DOCTOR 3
#define MAX_NURSE 5
#define MAX_XRAY 2
#define MAX_EXAM 5
#define XRAY_IMAGE 3
#define MAX_MEM 3000

/* 
 *	Global data structures.
 *	1: Examination Sheet.
 *	2: Enumerations. 
 */
/* Struct for Examination Sheet. */
struct ExamSheet{
	int     patientID;	/* binded with the Patient. */
	int     examRoomID;	/* binded with the Exam Room in the first time. */
	int     xrayID;	/* binded with the Xray Technician. */
	int     age;	/* >= 15 Adult, < 15 Child. */
	int     xray;
	int     shot;
	int     goToCashier;
	int     price;
	int     numberOfXray;
	char* name;
	char* xrayImage[XRAY_IMAGE];	/* 'nothing', 'break', 'compound fracture'. */
	char* symptom;
	char* result;	/* examination result. */
};

/* Enumerations for the state of each entity. */
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
	E_BUSY, E_FREE, E_READY, E_FINISH	/* E_READY: the Examination Room is ready for Doctor; E_FINISH: the Examination Room is ready (finish) for Nurse. */
};
enum ExamRoom_Task{
	E_FIRST, E_SECOND	/* the first or the second time the Patient meets with the Doctor. */
};
enum Xray_State{
	X_BUSY, X_FREE, X_FINISH
};
enum Cashier_State{
	C_BUSY, C_FREE
};
enum ParentChild_State{	/* for parent and child interaction. */
	P_BEGIN, P_XRAY, P_TABLE, P_EXAMXRAY, P_EXAMSHOT, P_EXAMDIAGNOSED, P_MOVE
};

/* 
 *Global variables.
 *	1: Entity numbers from the input.
 *	2: Monitor Variables.
 *	3: Locks.
 *	4: Condition Variables.
 *	5: Examination Sheets.
 *	6: Entity states.
 *	7: Entity tasks.
 *	8: Other Entities.
 */
/* Input numbers for each entity. */
int numberOfDoctors;
int numberOfNurses;
int numberOfExamRooms;
int numberOfWRNs;	/* waiting room nurse (1). */
int numberOfCashiers;	/* 1. */
int numberOfXrays;
int numberOfPatients;	/* Patients = Adults + Children. */
int numberOfAdults;
int numberOfChildren;
int numberOfParents;	/* Children = Parents. */

/* Monitor Variables for each entity. */
int leave[MAX_PATIENT];
int numOfNursesPassed; /* in total, how many Nurses have gone to take Patients. */
int numOfPatientsPassed;	/* equals to twice the number of the Patients. */
int nextActionForNurse; /* the state to indicate a Nurse is to pick up a Patient or not. */
int indexOfSheet;	/* Monitor Variable: the index of the sheet kept in WRN, initial value: 0. */
int indexOfPatient;	/* Monitor Variable: index Of Patient in the front of the waiting queue waiting for the Nurse, initial value: 0. */
int wrnNurseIndex;	/* the id of the nurse who are interacting  with the patient in waitingRoom. */
int wrnPatientID;
int wrnPatientAge;
int patientID[MAX_NURSE];
int patientWaitingCount;	/* counting how many Patients are waiting in line. */
int patientWaitNurseCount; /* the number of Patients waiting for the Nurse. */
int nurseWaitPatientCount;	/* the number of Nurses waiting for the Patient. */
int nurseWaitWrnCount;	/* the number of nurses waiting for the waiting room nurse. */
int examRoomNurseID[MAX_EXAM];
int examRoomDoctorID[MAX_EXAM];
int examRoomPatientID[MAX_EXAM];
int examRoomSecondTimeID[MAX_XRAY];	/* the ID for the Examination Room in the second visit. */
int xrayRoomID[MAX_XRAY];
int xrayPatientID[MAX_XRAY];
int parentTellChildDocID[MAX_MEM];	/* dynamic allocation. */
int childEnter[MAX_MEM];	/* dynamic allocation. */
int nurseTakeSheetID[10*MAX_PATIENT];	/* List. */
int head = 0, tail = 0;
int xrayWaitList[MAX_XRAY];
int cashierWaitList;

/* Locks for each entity. */
int WRLock;	/* set the lock for entering the Waiting Room. */
int WRInteractLock;	/* set the lock for the Waiting Room Nurse and the Patient to interact. */
int patientWaitNurseLock;	/* the lock of ready Patient waiting for the Nurse to escort. */
int nurseLock[MAX_NURSE];	/* the lock for the interaction between Nurse and Patient. */
int nurseWrnLock;
int examRoomLock;
int examRoomLockArray[MAX_EXAM];
int xrayCheckingLock;
int xrayWaitingLock[MAX_XRAY];
int xrayInteractLock[MAX_XRAY];	/* there could be MAX_XRAY (2) Xray Technicians. */
int cashierWaitingLock;
int cashierInteractLock;	/* there is just one single Cashier. */
int cabinetLock;
int parentChildLock[MAX_PATIENT];
int wrnWaitNurseLock;
int parentWaitChildLock[MAX_PATIENT];

/* Condition Variables for each entity. */
int WRInteractCV;		/* set the Condition Variable for the Waiting Room Nurse and the Patient to interact. */
int patientWaitingCV;	/* set the Condition Variable that Patients are waiting for in line. */
int patientWaitNurseCV;  /* the Condition Variable for the Monitor. */
int nurseCV[MAX_NURSE];		/* the Condition Variable for the interaction between Nurse and Patient. */
int nurseWrnCV;		/* the Condition Variable for Nurses waiting for the Waiting Room Nurse. */
int nurseWaitPatientCV;
int examRoomCVArray[MAX_EXAM];
int xrayWaitingCV[MAX_XRAY];
int xrayInteractCV[MAX_XRAY];
int cashierWaitingCV;
int cashierInteractCV;
int parentChildCV[MAX_PATIENT];
int wrnWaitNurseCV;
int parentWaitChildCV[MAX_PATIENT];

/* Examination Sheets for each entity. */
ExamSheet* wrnExamSheet;		/* the Waiting Room Nurse makes a new exam sheet for a new Patient. */
ExamSheet* nurseExamSheet[MAX_NURSE];
ExamSheet* examSheetArray[MAX_PATIENT]; /* the Exam Sheet binded with different Exam Rooms. */
ExamSheet* examRoomExamSheet[MAX_EXAM];	/* the current Examination Sheet from the Patient in Doctor(). */
ExamSheet* xrayExamSheet[MAX_XRAY];	/* the current Examination Sheet from the Patient in XrayTechnician(). */
ExamSheet* cashierExamSheet;	/* the current Examination Sheet from the Patient in Cashier(). */

/* States for each entity. */
WaitingRoomNurse_State waitingRoomNurseState;
NurseInteract_State isNurseInteract;
ExamRoom_State examRoomState[MAX_EXAM];
Xray_State xrayState[MAX_XRAY];
Cashier_State cashierState;
ParentChild_State parentChildState[MAX_PATIENT];

/* Tasks for each entity. */
WRNurse_Task WRNurseTask;
ExamRoom_Task examRoomTask[MAX_EXAM];

/* the current remaining Patient number. */
int remainPatientCount;
int remainNurseCount;

/* Index for each entity. */
int indexDoctor = 0;
int indexNurse = 0;
int indexWRN = 0;
int indexCashier = 0;
int indexXT = 0;
int indexPatient = 0;
int indexParent = 0;
int indexChild = 0;

/*
 * Init
 *	Initialize all of the global variables.
 *	Locks.
 *	Condition Variables.
 *	Monitor Variables.
 *	Entity State.
 */
void Init()
{
	int i = 0;
	
	/* Initialize the Waiting Room Nurse(). */
	wrnNurseIndex = 0;
	wrnPatientID = 0;
	wrnPatientAge = 0;
	indexOfSheet = 0;
	indexOfPatient = 0;
	numOfPatientsPassed = 0;
	WRLock = CreateLock("WRLock", sizeof("WRLock"));
	WRInteractLock = CreateLock("WRInteractLock", sizeof("WRInteractLock"));
	WRInteractCV = CreateCondition("WRInteractCV", sizeof("WRInteractCV"));
	waitingRoomNurseState = W_BUSY;
	
	/* Initialize the Patient(). */
	patientWaitingCount = 0;
	patientWaitNurseCount = 0;
	patientWaitNurseLock = CreateLock("patientWaitNurseLock", sizeof("patientWaitNurseLock"));
	patientWaitingCV = CreateCondition("patientWaitingCV", sizeof("patientWaitingCV"));
	patientWaitNurseCV = CreateCondition("patientWaitNurseCV", sizeof("patientWaitNurseCV"));
	wrnWaitNurseLock = CreateLock("wrnWaitNurseLock", sizeof("wrnWaitNurseLock"));
	wrnWaitNurseCV = CreateCondition("wrnWaitNurseCV", sizeof("wrnWaitNurseCV"));
	
	/* Initialize the Nurse(). */
	nurseWaitPatientCount = 0;
	numOfNursesPassed = 0;
	nurseWaitWrnCount = 0;
	nextActionForNurse = 0;
	for(i = 0; i < MAX_PATIENT; ++i){
		nurseTakeSheetID[i] = -1;
	}
	for(i = 0; i < MAX_NURSE; ++i){
		nurseLock[i] = CreateLock("nurseLock", sizeof("nurseLock"));
	}
	for(i = 0; i < MAX_NURSE; ++i){
		nurseCV[i] = CreateCondition("nurseCV", sizeof("nurseCV"));
	}
	nurseWrnLock = CreateLock("nurseWrnLock", sizeof("nurseWrnLock"));
	nurseWrnCV = CreateCondition("nurseWrnCV", sizeof("nurseWrnCV"));
	nurseWaitPatientCV = CreateCondition("nurseWaitPatientCV", sizeof("nurseWaitPatientCV"));
	isNurseInteract = N_FREE;
	
	/* Initialize the Doctor(). */
	examRoomLock = CreateLock("examRoomLock", sizeof("examRoomLock"));
	for(i = 0; i < MAX_EXAM; ++i){
		examRoomLockArray[i] = CreateLock("examRoomLock", sizeof("examRoomLock"));
	}
	for(i = 0; i < MAX_EXAM; ++i){
		examRoomCVArray[i] = CreateCondition("examRoomCVArray", sizeof("examRoomCVArray"));
	}
	for(i = 0; i < MAX_EXAM; ++i){
		examRoomState[i] = E_FREE;
	}
	for(i = 0; i < MAX_EXAM; ++i){
		examRoomTask[i] = E_FIRST;
	}
	
	/* Initialize the Cashier(). */
	cashierWaitingLock = CreateLock("cashierWaitingLock", sizeof("examRoomCVArray"));
	cashierInteractLock = CreateLock("cashierInteractLock", sizeof("cashierInteractLock"));
	cashierWaitingCV = CreateCondition("cashierWaitingCV", sizeof("cashierWaitingCV"));
	cashierInteractCV = CreateCondition("cashierInteractCV", sizeof("cashierInteractCV"));
	cashierState = C_BUSY;
	cashierWaitList = 0;
	
	/* Initialize the XrayTechnician(). */
	xrayCheckingLock = CreateLock("xrayCheckingLock", sizeof("xrayCheckingLock"));
	for(i = 0; i < MAX_XRAY; ++i){
		xrayWaitingLock[i] = CreateLock("xrayWaitingLock", sizeof("xrayWaitingLock"));
	}
	for(i = 0; i < MAX_XRAY; ++i){
		xrayInteractLock[i] = CreateLock("xrayInteractLock", sizeof("xrayInteractLock"));
	}
	for(i = 0; i < MAX_XRAY; ++i){
		xrayWaitingCV[i] = CreateCondition("xrayWaitingCV", sizeof("xrayWaitingCV"));
	}
	for(i = 0; i < MAX_XRAY; ++i){
		xrayInteractCV[i] = CreateCondition("xrayInteractCV", sizeof("xrayInteractCV"));
	}
	for(i = 0; i < MAX_XRAY; ++i){
		xrayState[i] = X_BUSY;
	}
	for(i = 0; i < MAX_XRAY; ++i){
		xrayWaitList[i] = 0;
	}
	
	for(i = 0; i < numberOfPatients; ++i){
		parentChildState[i] = P_BEGIN;
	}
	for(i = 0; i < numberOfPatients; ++i){
		parentTellChildDocID[i] = 0;
	}
	for(i = 0; i < numberOfPatients; ++i){
		leave[i] = 0;
	}
	for(i = 0; i < numberOfPatients; ++i){
		parentChildLock[i] = CreateLock("parentChildLock", sizeof("parentChildLock"));
	}
	for(i = 0; i < numberOfPatients; ++i){
		parentChildCV[i] = CreateCondition("parentChildCV", sizeof("parentChildCV"));
	}
	for(i = 0; i < numberOfPatients; ++i){
		parentWaitChildLock[i] = CreateLock("parentWaitChildLock", sizeof("parentWaitChildLock"));
	}
	for(i = 0; i < numberOfPatients; ++i){
		parentWaitChildCV[i] = CreateCondition("parentWaitChildCV", sizeof("parentWaitChildCV"));
	}
	for(i = 0; i < numberOfPatients; ++i){
		childEnter[i] = 0;
	}
	
	/* Initialize other variables. */
	cabinetLock = CreateLock("cabinetLock", sizeof("cabinetLock"));
}

/* Doctor */
void Doctor()
{
	int i = 0, roomIndex = -1, result = -1, images = -1;
	int index = indexDoctor++;
	
	while(remainPatientCount){
		/* Check for the state of the Examination Rooms. */
		Acquire(examRoomLock);
		roomIndex = -1;
		for(i = 0; i < numberOfExamRooms; ++i){
			if(E_READY == examRoomState[i]){
				examRoomState[i] = E_BUSY;
				roomIndex = i; break;
			}
		}
		Release(examRoomLock);
		
		/* There is one room ready for the Doctor. */
		if(roomIndex != -1){
			Acquire(examRoomLockArray[roomIndex]);
			/* Wake up the waiting Patient and Nurse. */
			examRoomDoctorID[roomIndex] = index;	/* set the Doctor ID for the current Examination Room. */
			BroadCast(examRoomLockArray[roomIndex], examRoomCVArray[roomIndex]);
			Wait(examRoomLockArray[roomIndex], examRoomCVArray[roomIndex]);	/* wait to be waked up by the Patient. */
	
			if(E_FIRST == examRoomTask[roomIndex]){
				print("Doctor [%d] is reading the examination sheet of [Adult/Child] Patient [%d] in Examination room [%d].\n", index, examRoomPatientID[roomIndex], roomIndex);
				result = rand() % 4;
				if(0 == result){	/* need an Xray. */
					images = rand() % 3;
					print("Doctor [%d] notes down in the sheet that Xray is needed for [Adult/Child] Patient [%d] in Examination room [%d].\n", index, examRoomPatientID[roomIndex], roomIndex);
					examRoomExamSheet[roomIndex]->xray = 1;
					examRoomExamSheet[roomIndex]->numberOfXray = images + 1;
				}else if(1 == result){	/* need a shot. */
					print("Doctor [%d] notes down in the sheet that [Adult/Child] Patient [%d] needs to be given a shot in Examination room [%d].\n", index, examRoomPatientID[roomIndex], roomIndex);
					examRoomExamSheet[roomIndex]->shot = 1;
				}else{	/* fine and can leave. */
					print("Doctor [%d] diagnoses [Adult/Child] Patient [%d] to be fine and is leaving Examination Room [%d].\n", index, examRoomPatientID[roomIndex], roomIndex);
				}
				Signal(examRoomLockArray[roomIndex], examRoomCVArray[roomIndex]);
			}else if(E_SECOND == examRoomTask[roomIndex]){
				print("Doctor [%d] is examining the Xrays of [Adult/Child] Patient [%d] in Examination room [%d].\n", index, examRoomPatientID[roomIndex], roomIndex);
				Signal(examRoomLockArray[roomIndex], examRoomCVArray[roomIndex]);
				Wait(examRoomLockArray[roomIndex], examRoomCVArray[roomIndex]);
				print("Doctor [%d] has left Examination Room [%d].\n", index, roomIndex);
				print("Doctor [%d] is going to their office.\n", index);
				Signal(examRoomLockArray[roomIndex], examRoomCVArray[roomIndex]);
			}
			/* Loop into the next iteration. */
			Release(examRoomLockArray[roomIndex]);
		}
		
		/* Avoid busy waiting. */
		for(i = 0; i < 100; ++i)
			Yield();
	}
	Exit(0);
}

/* Nurse */
void Nurse()
{
	int i = 0;
	int loopTime = 0;
	int loopRoomID = 0;
	int patient_ID = -1;
	int xrayLoop = 0;
	int index_xray = -1;
	int loopID = 0;
	int tempXrayID = 0;
	int index = indexNurse++;
    ExamSheet* patientExamSheet;
    
	while(remainPatientCount)
	{
		/*********************************************************/
		/* Task 1: escort the Patients from Waiting Room to the Exam Room. */
		Acquire(examRoomLock);	/* go to see if there is any Doctor/Examination Room available. */
	    for(i = 0; i < numberOfExamRooms; ++i)	/* check to see which Examination Room is avaiable. */
            if(examRoomState[i] == E_FREE)
            {
			    break;
			}
        if(i == numberOfExamRooms)	/* no Examination Room is availble at this time. */
		{
		    Release(examRoomLock);
		}
		else
		{		    
			examRoomState[i] = E_BUSY; /*set the state of the examRoom to be E_BUSY. */
			Release(examRoomLock);
			Acquire(nurseWrnLock);
			nurseTakeSheetID[tail++] = index;
			nurseWaitWrnCount++;
			Wait(nurseWrnLock, nurseWrnCV);
			
			Acquire(wrnWaitNurseLock);
			nurseWaitWrnCount--;
			if(nextActionForNurse == 1)    /*when the nurse is informed by the WRN to go to take a patient in the waiting room. */
			{
				patientExamSheet = wrnExamSheet;				
				Release(nurseWrnLock);
				Acquire(patientWaitNurseLock);   /*Nurse acquires the lock to escort patient. */
				nurseWaitPatientCount++;
				if(isNurseInteract == N_BUSY)     
				{
					Wait(patientWaitNurseLock, nurseWaitPatientCV); /*Last nurse is still interacting with a patient in the lock, so this Nurse waits here. */
				}
				isNurseInteract = N_BUSY;  
				nurseWaitPatientCount--;  /*minuse the number of nurses who are waiting for the patient. */
				patientWaitNurseCount--;  /*minuse the number of patients who are waiting for the nurse to escort to the examRoom. */
				wrnNurseIndex = index; 	/* the Nurse would escort the Patient. */
				Signal(patientWaitNurseLock, patientWaitNurseCV);
			
				Acquire(nurseLock[index]);  
				Release(patientWaitNurseLock);
				
				/*awake the waiting nurse and let she continue her work. */
				Signal(wrnWaitNurseLock, wrnWaitNurseCV);
				Release(wrnWaitNurseLock);
				
				Wait(nurseLock[index], nurseCV[index]);
				print("Nurse [%d] escorts Adult Patient /Parent [%d] to the examination room.\n", index, patientID[index]);
			
				patient_ID = patientID[index];				
				nurseExamSheet[index]->examRoomID = i;	/* assign the Exam Room number. */
				Signal(nurseLock[index], nurseCV[index]);
				Acquire(examRoomLockArray[i]); /*acquire the lock of the examRoom. */
				Release(nurseLock[index]);
				Wait(examRoomLockArray[i], examRoomCVArray[i]);  /*wait the patient to be prepared to be taken test. */
			    print("Nurse [%d] takes the temperature and blood pressure of Adult Patient /Parent [%d].\n", index, patientID[index]);
				print("Nurse [%d] asks Adult Patient /Parent [%d] What Symptoms do you have?\n", index, patientID[index]);
                Signal(examRoomLockArray[i], examRoomCVArray[i]); 
				Wait(examRoomLockArray[i], examRoomCVArray[i]); 
				print("Nurse [%d] writes all the information of Adult Patient /Parent [%d] in his examination sheet.\n", index, patientID[index]);
                Signal(examRoomLockArray[i], examRoomCVArray[i]);
			    Wait(examRoomLockArray[i], examRoomCVArray[i]);	
				print("Nurse [%d] informs Doctor [%d] that Adult/Child Patient [%d] is waiting in the examination room [%d].\n", index,examRoomDoctorID[i],patient_ID,i);
				print("Doctor [%d] is leaving their office.\n", examRoomDoctorID[i]);	/* go to the Exam Room from Office. */
				print("Nurse [%d] hands over to the Doctor [%d] the examination sheet of Adult/Child Patient [%d].\n", index,examRoomDoctorID[i],patient_ID);
				Release(examRoomLockArray[i]);			
			}
		    else
			{
			    Acquire(examRoomLock);
				examRoomState[i] = E_FREE;
				Release(examRoomLock);
				Release(nurseWrnLock);
				Signal(wrnWaitNurseLock, wrnWaitNurseCV);
				Release(wrnWaitNurseLock);
			}
		}
		/* end of Task 1. */

		/* Avoid busy waiting. */
		for(loopTime = 0; loopTime != 100; ++loopTime)
			Yield();
			
		/****************************************************/
		/* Task 2: escort the Patients from Exam Room to Xray Room. */
		Acquire(examRoomLock);	/* to go to get a Patient. */
		/* the id for examRoomID. */
		for(loopRoomID = 0; loopRoomID < numberOfExamRooms; ++loopRoomID)
		{
			print("Nurse [%d] checks the wall box of examination room [%d].\n", index,loopRoomID);
			if(examRoomState[loopRoomID] == E_FINISH)
			{
				examRoomState[loopRoomID] = E_BUSY;
				break;
			}
		}
		Release(examRoomLock);	 

		if(loopRoomID != numberOfExamRooms)	/* when there is Patient with the state of FINISH. */
		{
			Acquire(examRoomLockArray[loopRoomID]);
			patientExamSheet = examRoomExamSheet[loopRoomID];
			if(1 == patientExamSheet->xray && (0 == patientExamSheet->goToCashier))
			{
				examRoomNurseID[loopRoomID] = index;	
				Signal(examRoomLockArray[loopRoomID], examRoomCVArray[loopRoomID]);	/* signal a person in the Examination Room. */
				patientExamSheet = examRoomExamSheet[loopRoomID];
				index_xray = -1;
				Acquire(xrayCheckingLock);	/* check the state of the Xray Room. */
				for(xrayLoop = 0; xrayLoop != numberOfXrays; ++xrayLoop)
				{
					if(xrayState[xrayLoop] == X_FREE)
					{
						xrayState[xrayLoop] = X_BUSY;
						break;
					}
				}
				Release(xrayCheckingLock);
				if(xrayLoop != numberOfXrays)	/* there is FREE Xray Room. */
				{
					xrayRoomID[loopRoomID] = xrayLoop;
				}else{
					index_xray = rand() % numberOfXrays;
					xrayRoomID[loopRoomID] = index_xray;
				}
				print("Nurse [%d] escorts Adult Patient /Parent [%d] to the X-ray room [%d].\n", index, patientExamSheet->patientID,index_xray);
				print("Nurse [%d] informs X-Ray Technician [%d] about Adult/Child Patient [%d] and hands over the examination sheet.\n", index,loopRoomID,patientExamSheet->patientID);
				print("Nurse [%d] leaves the X-ray Room [%d].\n", index,loopRoomID);
			}
			else if(1 == patientExamSheet->shot && (0 == patientExamSheet->goToCashier))
			{
				examRoomNurseID[loopRoomID] = index;	
				Signal(examRoomLockArray[loopRoomID], examRoomCVArray[loopRoomID]);	/* signal a person in the Examination Room. */
				patientExamSheet = examRoomExamSheet[loopRoomID];
				print("Nurse [%d] goes to supply cabinet to give to take medicine for Adult Patient /Parent [%d].\n", index, patientExamSheet->patientID);
				Acquire(cabinetLock);
				/* do sth in Cabinet. */
				cabinetLock->Release();
				print("Nurse [%d] asks Adult Patient/Parent [%d] \"Are you ready for the shot?\"\n", index, patientExamSheet->patientID);
				Wait(examRoomLockArray[loopRoomID], examRoomCVArray[loopRoomID]);
				print("Nurse [%d] tells Adult Patient/Parent [%d] \"Your shot is over\".\n", index, patientExamSheet->patientID);
				Signal(examRoomLockArray[loopRoomID], examRoomCVArray[loopRoomID]);
			}else{
				Acquire(examRoomLock);
				examRoomState[loopRoomID] = E_FINISH;
				Release(examRoomLock);				
			}
			Release(examRoomLockArray[loopRoomID]);
		}	
		else{	/* no Patient is in state of FINISH. */
		}		

		for(loopTime = 0; loopTime != 100; ++loopTime)
			Yield();

		/******************************************/
		/* Task 3: escort from Xray Room to Exam Room. */
		Acquire(examRoomLock);
		/*	to search a free room in the examRoomLock. */
		for(loopID = 0; loopID < numberOfExamRooms; ++loopID)
		{
			if(examRoomState[loopID] == E_FREE)
			{
				examRoomState[loopID] = E_BUSY;
				break;
			}	
		}
		Release(examRoomLock);

		if(loopID != numberOfExamRooms) /* there is a FREE Exam Room. */
		{
			Acquire(xrayCheckingLock);
			tempXrayID = 0;
			for(; tempXrayID < numberOfXrays; ++tempXrayID)
			{
				if(xrayState[tempXrayID] == X_FINISH)
				{
					xrayState[tempXrayID] = X_BUSY;
					break;
				}
			}
			Release(xrayCheckingLock);
			if(tempXrayID != numberOfXrays)	/* one patient finishs in a xrayRoom. */
			{
				Acquire(xrayInteractLock[tempXrayID]);
				examRoomSecondTimeID[tempXrayID] = loopID;
				BroadCast(xrayInteractLock[tempXrayID], xrayInteractCV[tempXrayID]);
				patientExamSheet = xrayExamSheet[tempXrayID];
				print("Nurse [%d] gets examination sheet for [Adult Patient/Parent] [%d] in Xray waiting room.\n", index,patientExamSheet->patientID);
				Acquire(examRoomLockArray[loopID]);
				print("Nurse [%d] escorts Adult Patient/Parent [%d] to the examination room [%d].\n", index,patientExamSheet->patientID,loopID);
				Release(xrayInteractLock[tempXrayID]);
				examRoomCVArray[loopID]->Wait(examRoomLockArray[loopID]);
				print("Nurse [%d] informs Doctor [%d] that Adult/Child Patient [%d] is waiting in the examination room [%d].\n", index,examRoomDoctorID[loopID],patientExamSheet->patientID,loopID);
				print("Doctor [%d] is leaving their office.\n", examRoomDoctorID[loopID]);	/* go to the Exam Room from Office. */
				print("Nurse [%d] hands over to the Doctor [%d] the examination sheet of Adult/Child Patient [%d].\n", index,examRoomDoctorID[loopID],patientExamSheet->patientID);
				Release(examRoomLockArray[loopID]);
			}else{
				Acquire(examRoomLock);
				examRoomState[loopID] = E_FREE;
				Release(examRoomLock);
			}
		}
        /* end of Task 3. */		
		
		for(loopTime = 0; loopTime != 100; ++loopTime)
			Yield();
		
		/*************************************************/
		/* Task 4: go to make sure that the Patient is go to Cahier. */
		Acquire(examRoomLock);
		/* to search a free room in the examRoomLock. */
		for(loopID = 0; loopID < numberOfExamRooms; ++loopID)
		{
			if(examRoomState[loopID] == E_FINISH)
			{
				examRoomState[loopID] = E_BUSY;
				break;
			}	
		}
		Release(examRoomLock);

		if(loopID != numberOfExamRooms)
		{
			Acquire(examRoomLockArray[loopID]);
			if(1 == examRoomExamSheet[loopID]->goToCashier)
			{
				print("Nurse [%d] escorts Adult Patient /Parent [%d] to Cashier.\n", index, examRoomExamSheet[loopID]->patientID);
				examRoomCVArray[loopID]->Signal(examRoomLockArray[loopID]);
			}
			else
			{
				Acquire(examRoomLock);
				examRoomState[loopID] = E_FINISH;
				Release(examRoomLock);				
			}
			Release(examRoomLockArray[loopID]);
		}

		for(loopTime = 0; loopTime != 100; ++loopTime)
			Yield();   		
	}
	remainNurseCount--;
	Exit(0);
}

/* WaitingRoomNurse */
void WaitingRoomNurse() 
{
	int i = 0;
	int patientAge = -1;
	int nurseTakeSheet_ID = -1;
	int index = indexWRN++;
	
	while (remainPatientCount || remainNurseCount) {
		if(numOfPatientsPassed < numberOfPatients){
			Acquire(WRLock);
			if (patientWaitingCount > 0) {		/* a Patient is in line. */
				Signal(WRLock, patientWaitingCV);		/* the Waiting Room Nurse wake a Patient up. */
				patientWaitingCount--;
				waitingRoomNurseState = W_BUSY;		/* make the Waiting Room Nurse busy when she wakes up a Patient. */
			}else {
				waitingRoomNurseState = W_FREE;		/* if no one is in line, the Waiting Room Nurse is free. */
			}
			
			Acquire(WRInteractLock);		/* the Waiting Room Nurse gets ready for talking with Patient first. */
			Release(WRLock);		/* let the Patient have the right to act. */
			Wait(WRInteractLock, WRInteractCV);		/* wait for the Patient coming to tell the task. */
			/* see what task to perform. */
			
			if (WRNurseTask == W_GETFORM) {
				wrnExamSheet = (ExamSheet*)malloc(sizeof(ExamSheet));	/* make a new exam sheet. */
				wrnExamSheet->xray = 0;
				wrnExamSheet->shot = 0;
				wrnExamSheet->goToCashier = 0;
				patientAge = wrnPatientAge;
				if (patientAge > 14) {
					print("Waiting Room nurse gives a form to Adult patient [%d].\n", wrnPatientID);
				}else {
					print("Waiting Room nurse gives a form to the Parent of Child patient [%d].\n", wrnPatientID);
				}
				Signal(WRInteractLock, WRInteractCV);		/* tell the Patient that she makes a new sheet. */
				Wait(WRInteractLock, WRInteractCV);		/* wait for Patient to tell her they get the sheet. */
			}		/* done with a Patient's getting form task. */
			else if (WRNurseTask == W_GIVEFORM) {
				print("Waiting Room nurse accepts the form from Adult Patient/Parent with name [%s] and age [%d].\n", wrnExamSheet->name, wrnExamSheet->age);
				examSheetArray[indexOfSheet++] = wrnExamSheet;	/* dynamic allocation. */
				print("Waiting Room nurse creates an examination sheet for [Adult/Child] patient with name [%s] and age [%d].\n", wrnExamSheet->name, wrnExamSheet->age);
				print("Waiting Room nurse tells the Adult Patient/Parent [%d] to wait in the waiting room for a nurse.\n", wrnExamSheet->patientID);
			}
			Release(WRInteractLock);		
		}
		
		/* Waiting Room Nurse tries to get the nurseWrnLock to give an Exam Sheet to the Nurse. */
		Acquire(nurseWrnLock);
		if(nurseWaitWrnCount > 0) {
			/* there's Nurse waiting for escorting Patient. */
			if(indexOfSheet > numOfNursesPassed)
			{
				/* there are more registered Patients than Nurses. */
				wrnExamSheet = examSheetArray[indexOfPatient++];
				nurseTakeSheet_ID = nurseTakeSheetID[head++];
				numOfNursesPassed++;
				nextActionForNurse = 1; /* tell Nurse to go to pick up a Patient. */
				
				print("Nurse [%d] tells Waiting Room Nurse to give a new examination sheet.\n", nurseTakeSheet_ID);
				print("Waiting Room nurse gives examination sheet of patient [%d] to Nurse [%d].\n", wrnExamSheet->patientID, nurseTakeSheet_ID);
				Signal(nurseWrnLock, nurseWrnCV);
				/* WRN tries to release nurseWrnLock after giving the Exam Sheet to a Nurse. */
				Acquire(wrnWaitNurseLock);
				Release(nurseWrnLock);		
				Wait(wrnWaitNurseLock, wrnWaitNurseCV);
				Release(wrnWaitNurseLock);
			}
			else
			{
				/* WRN finds that there is no Patient waiting for the Nurse or there is enough Nurses for the Patient. */
				nurseTakeSheet_ID = nurseTakeSheetID[head++];
				nextActionForNurse = 0; /* not go to pick Patient. */
				Signal(nurseWrnLock, nurseWrnCV);
				/* WRN tries to release nurseWrnLock because there is no Patient waiting. */
				Acquire(wrnWaitNurseLock);
				Release(nurseWrnLock);
				Wait(wrnWaitNurseLock, wrnWaitNurseCV);
				Release(wrnWaitNurseLock);
			}
		}
		else
		{	/* no Nurse waits in line. */
			/* WRN tries to release nurseWrnLock because there is no Nurse waiting. */
			Release(nurseWrnLock);
		}
		
		for(i = 0 ; i < 100; ++i)
			Yield();
	}	/*end of while */
	Exit(0);
}

/* Cashier */
void Cashier()
{
	int i = 0;
	int index = indexCashier++;
	
	while(remainPatientCount){
		if(cashierWaitList != 0){
			/*************************************/
			/* Try to enter the Cashier waiting section. */
			/* There may be some Patients waiting for the Cashier. */
			/* Start of section 1 in Cashier(). */
			Acquire(cashierWaitingLock);
			if(cashierWaitList != 0){
				/* Some Patients are in line. */
				/* Wake them up. */
				Signal(cashierWaitingLock, cashierWaitingCV);
				--cashierWaitList;
				cashierState = C_BUSY;
			}
			
			/**************************************/
			/* Try to enter the Cashier interact section.  */
			/* There may be one Patient to interact with the Cashier. */
			/* Start of section 2 in Cashier(). */
			Acquire(cashierInteractLock);
			/********************************/
			/* Leave the Cashier waiting section. */
			/* End of section 1 in Cashier(). */
			Release(cashierWaitingLock);
			
			Wait(cashierInteractLock, cashierInteractCV);
			cashierExamSheet->price = rand() % 1000 + 50;	/* random the price. */
			if(cashierExamSheet->age < 15){	/* for Child Patient. */
				print("Cashier receives the examination sheet for Child Patient [%d] from Parent [%d].\n", cashierExamSheet->patientID, cashierExamSheet->patientID);
				print("Cashier reads the examination sheet of Child Patient [%d] and asks Parent [%d] to pay $%d.\n", cashierExamSheet->patientID, cashierExamSheet->patientID, cashierExamSheet->price);
				Signal(cashierInteractLock, cashierInteractCV);
				Wait(cashierInteractLock, cashierInteractCV);
				print("Cashier accepts $%d from Parent [%d].\n",  cashierExamSheet->price, cashierExamSheet->patientID);
				print("Cashier gives a receipt of $%d to Parent [%d].\n", cashierExamSheet->price, cashierExamSheet->patientID);
				Signal(cashierInteractLock, cashierInteractCV);
				Wait(cashierInteractLock, cashierInteractCV);
				print("Cashier waits for the Patient to leave.\n");
				Signal(cashierInteractLock, cashierInteractCV);
				Wait(cashierInteractLock, cashierInteractCV);		
			}else{	/* for Adult Patient. */
				print("Cashier receives the examination sheet from Adult Patient [%d].\n", cashierExamSheet->patientID);
				print("Cashier reads the examination sheet of Adult Patient [%d] and asks him to pay $%d.\n", cashierExamSheet->patientID, cashierExamSheet->price);
				Signal(cashierInteractLock, cashierInteractCV);
				Wait(cashierInteractLock, cashierInteractCV);
				print("Cashier accepts $%d from Adult Patient [%d].\n",  cashierExamSheet->price, cashierExamSheet->patientID);
				print("Cashier gives a receipt of $%d to Adult Patient [%d].\n", cashierExamSheet->price, cashierExamSheet->patientID);
				Signal(cashierInteractLock, cashierInteractCV);
				Wait(cashierInteractLock, cashierInteractCV);
				print("Cashier waits for the Patient to leave.\n");
				Signal(cashierInteractLock, cashierInteractCV);
				Wait(cashierInteractLock, cashierInteractCV);			
			}
			
			/********************************/
			/* Leave the Cashier interact section. */
			/* End of section 2 in Cashier(). */
			Release(cashierInteractLock);
		}else{
			/* No patients are in line. */
			/*cashierState = C_FREE; */
		}
		
		/* Avoid busy waiting. */
		for(i = 0; i < 100; ++i)
			Yield();
	}
	Exit(0);
}

/* XrayTechnician */
void XrayTechnician()
{
	int i = 0, result = -1;
	int index = indexXT++; 
	
	while(remainPatientCount){
		/***********************************/
		/* Try to enter the Xray waiting section. */
		/* Start of section 1 in XrayTechnician(). */
		/*if(!xrayWaitList[index]->IsEmpty()){*/
		if(xrayWaitList[index] != 0){
			Acquire(xrayWaitingLock[index]);
			/* There is one room waiting for Xray Technician. */
			if(xrayWaitList[index] != 0){
				Signal(xrayWaitingLock[index], xrayWaitingCV[index]);
				--xrayWaitList[index];
				xrayState[index] = X_BUSY;
			}
			
			/************************************/
			/* Try to enter the Xray interact section. */
			/* Start of section 2 in XrayTechnician(). */
			Acquire(xrayInteractLock[index]);
			/******************************/
			/* Leave the Xray waiting section. */
			/* End of section 1 in XrayTechnician(). */
			Release(xrayWaitingLock[index]);
			Wait(xrayInteractLock[index], xrayInteractCV[index]);
			for(i = 0; i < xrayExamSheet[index]->numberOfXray; ++i){
				if(xrayExamSheet[index]->age < 15){	/* for Child Patient. */
					if(0 == i)	print("Xray technician [%d] asks Child Patient [%d] to get on the table.\n", index, xrayPatientID[index]);
					else print("Xray Technician [%d] asks Child Patient [%d] to move.\n", index, xrayPatientID[index]);
					Signal(xrayInteractLock[index], xrayInteractCV[index]);
					Wait(xrayInteractLock[index], xrayInteractCV[index]);
					print("Xray Technician [%d] takes an Xray Image of Child Patient [%d].\n", index, xrayPatientID[index]);
					/* Random the Xray result. */
					result = rand() % 3;
					if(0 == result) xrayExamSheet[index]->xrayImage[i] = "nothing";
					else if(1 == result) xrayExamSheet[index]->xrayImage[i] = "break";
					else if(2 == result) xrayExamSheet[index]->xrayImage[i] = "compound fracture";
					print("Xray Technician [%d] records [%s] on Child Patient [%d]'s examination sheet.\n", index, xrayExamSheet[index]->xrayImage[i], xrayPatientID[index]);
					Signal(xrayInteractLock[index], xrayInteractCV[index]);
					Wait(xrayInteractLock[index], xrayInteractCV[index]);
				}else{	/* for Adult Patient. */
					if(0 == i)	print("Xray technician [%d] asks Adult Patient [%d] to get on the table.\n", index, xrayPatientID[index]);
					else print("Xray Technician [%d] asks Adult Patient [%d] to move.\n", index, xrayPatientID[index]);
					Signal(xrayInteractLock[index], xrayInteractCV[index]);
					Wait(xrayInteractLock[index], xrayInteractCV[index]);
					print("Xray Technician [%d] takes an Xray Image of Adult Patient [%d].\n", index, xrayPatientID[index]);
					/* Random the Xray result. */
					result = rand() % 3;
					if(0 == result) xrayExamSheet[index]->xrayImage[i] = "nothing";
					else if(1 == result) xrayExamSheet[index]->xrayImage[i] = "break";
					else if(2 == result) xrayExamSheet[index]->xrayImage[i] = "compound fracture";
					print("Xray Technician [%d] records [%s] on Adult Patient [%d]'s examination sheet.\n", index, xrayExamSheet[index]->xrayImage[i], xrayPatientID[index]);
					Signal(xrayInteractLock[index], xrayInteractCV[index]);
					Wait(xrayInteractLock[index], xrayInteractCV[index]);
				}
			}
			
			/* For both of the Child and Adult Patients. */
			print("X-ray Technician [%d] tells [Adult Patient/Parent] [%d] to wait in Xray waiting room.\n", index, xrayPatientID[index]);
			print("X-ray Technician [%d] puts [Adult Patient/Parent] [%d] in Xray waiting room wall pocket.\n", index, xrayPatientID[index]);
			Signal(xrayInteractLock[index], xrayInteractCV[index]);
			Wait(xrayInteractLock[index], xrayInteractCV[index]);
			
			/*******************************/
			/* Leave the Xray Technician interact section in XrayTechnician[index]. */
			/* End of section 2 in XrayTechnician(). */
			Release(xrayInteractLock[index]);
		}else{
			/* No patients are in line. */
			/* xrayState[index] = X_FREE; */
		}
		
		/* Avoid busy waiting. */
		for(i = 0; i < 100; ++i)
			Yield();
	}
	Exit(0);
}

/* Patient */
void Patient() {
	int index = indexPatient++;	
	ExamSheet* myExamSheet;		/* the Patient declares a new Exam Sheet for himself. */
	char patient_id[10];
	char patient_name[20];
	int patient_ID = index;
	int patient_age = rand() % 50 + 15;
	int i = 0, symptomID = -1;
	int nurse_id = -1;
	int examRoom_id = -1;
	int examRoomSecondTime_id = -1;
	int examRoomNurse_id = -1;
	int xrayRoom_id = -1;
	
	print("Adult Patient [%d] has entered the Doctor's Office Waiting Room.\n", patient_ID);
	/* the Patient gets in the line for the first time to get a form. */
	Acquire(WRLock);		/* the Patient tries to enter the Waiting Room and stay in line. */
	print("Adult Patient [%d] gets in line of the Waiting Room Nurse to get registration form.\n", patient_ID);
	if (waitingRoomNurseState == W_BUSY) {		/* the Waiting Room Nurse is busy. then the Patient gets in line. */
		patientWaitingCount++;
		Wait(WRLock, patientWaitingCV);
	}else {
		waitingRoomNurseState = W_BUSY;
	}
	Release(WRLock);		/* the Patient finishes waiting and gets ready to interact with the Waiting Room Nurse. */
	
	Acquire(WRInteractLock);
	/*need to tell the Waiting Room Nurse what to do -- firstly get a form. */
	WRNurseTask = W_GETFORM;
	wrnPatientID = patient_ID;
	wrnPatientAge = patient_age;
	Signal(WRInteractLock, WRInteractCV);		/* tell the Waiting Room Nurse that he comes. */
	Wait(WRInteractLock, WRInteractCV);		/* wait for the Waiting Room Nurse to make a new sheet. */
	print("[Adult patient] gets the form from the Waiting Room Nurse.\n");
	myExamSheet = wrnExamSheet;		/* get the Exam Sheet. */
	Signal(WRInteractLock, WRInteractCV);		/* tell the Waiting Room Nurse that he gets the sheet. */
	Release(WRInteractLock);
	
	/* Patient goes away and fills the form. */
	myExamSheet->patientID = patient_ID;		/* write the Patient id, his name, on the sheet. */
	myExamSheet->age = patient_age;		/* write the age of the Patient on the Exam Sheet. Adult Patient age should be larger than 15. */
	sprint(patient_id, "%d ", patient_ID);
	strcpy(patient_name, "Patient_");
	strcat(patient_name, patient_id);
	myExamSheet->name = patient_name;		/* write the name string on the Exam Sheet struct for print. */
	
	/* the Patient gets back to the line to submit the form. */
	Acquire(WRLock);		/* the Patient tries to enter the Waiting Room and stay in line. */
	print("Adult Patient [%d] gets in line of the Waiting Room Nurse to submit registration form.\n", patient_ID);
	if (waitingRoomNurseState == W_BUSY) {		/* the Waiting Room Nurse is busy. then the Patient gets in line. */
		patientWaitingCount++;
		Wait(WRLock, patientWaitingCV);
	}else {
		waitingRoomNurseState = W_BUSY;
	}
	Release(WRLock);		/* the Patient finishes waiting and gets ready to interact with the Waiting Room Nurse. */
	
	Acquire(WRInteractLock);
	/* need to tell the Waiting Room Nurse what to do -- secondly turn over a form. */
	WRNurseTask = W_GIVEFORM;
	print("[Adult patient] submits the filled form to the Waiting Room Nurse.\n");
	wrnExamSheet = myExamSheet;
	numOfPatientsPassed++;	/* one Patient passed the interaction with the Waiting Room Nurse. */
	Signal(WRInteractLock, WRInteractCV);		/* turn over the filled form. */
	
	/* Patient waits for a Nurse in line. */
	Acquire(patientWaitNurseLock);
	patientWaitNurseCount++;
	Release(WRInteractLock);		/* Patient leaves the Waiting Room Nurse. */
	
	Wait(patientWaitNurseLock, patientWaitNurseCV);		/* Patient waits for a Nurse come to escort him. */
	if(nurseWaitPatientCount > 0)
	{	/* before leaving, wake up the next Nurse in line to get prepared for escorting the next Patient in line away. */
		Signal(patientWaitNurseLock, nurseWaitPatientCV);
		isNurseInteract = N_BUSY;
	}
	else
	{
		isNurseInteract = N_FREE;
	}
	
	/* escorted away by the Nurse. */
	nurse_id = wrnNurseIndex;		/* Patient knows which Nurse escorts him/her away. */
	Release(patientWaitNurseLock);
	
	/* Patient tries to acquire the lock for Nurse. */
	Acquire(nurseLock[nurse_id]);
	patientID[nurse_id] = patient_ID;
	nurseExamSheet[nurse_id] = myExamSheet;
	Signal(nurseLock[nurse_id], nurseCV[nurse_id]);
	Wait(nurseLock[nurse_id], nurseCV[nurse_id]);		/* waiting for the Nurse to get his Exam Sheet. */
	myExamSheet = nurseExamSheet[nurse_id];		/* get information back from the Nurse and know symptoms. */
	examRoom_id = myExamSheet->examRoomID;
	Release(nurseLock[nurse_id]);
	
	/* Patient enters the Exam Room. */
	
	Acquire(examRoomLockArray[examRoom_id]);
	print("[Adult Patient] [%d] is following Nurse [%d] to Examination Room [%d].\n", patient_ID, nurse_id, examRoom_id);
	print("[Adult Patient] [%d] has arrived at Examination Room [%d].\n", patient_ID, examRoom_id);
	Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	symptomID = rand()%3;
	if(symptomID == 0)
		myExamSheet->symptom = "pain";
	else if(symptomID == 1)
		myExamSheet->symptom = "nausea";
	else
		myExamSheet->symptom = "hear alien voices";
	print("Adult Patient [%d] says, \"My symptoms are [%s]\".\n", patient_ID, myExamSheet->symptom);
	examRoomExamSheet[examRoom_id] = myExamSheet;
	Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	Acquire(examRoomLock);
	examRoomState[examRoom_id] = E_READY;
	examRoomTask[examRoom_id] = E_FIRST;
	Release(examRoomLock);
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	
	/* turn over the exam sheet to the Doctor. */
	examRoomPatientID[examRoom_id ] = index;
	Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		
	myExamSheet = examRoomExamSheet[examRoom_id];		/* Doctor finishes the first time Examination. */
		
	if (1 == myExamSheet->xray) {	/* the Patient needs to take an Xray. */
		print("Adult Patient [%d] has been informed by Doctor [%d] that he needs an Xray.\n", patient_ID, examRoomDoctorID[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		Release(examRoomLock);
		print("Adult Patient [%d] waits for a Nurse to escort them to the Xray room.\n", patient_ID);
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);	/* the Patient waits for a Nurse to escort him/her to Xay Room. */
		
		/* a Nurse comes to escort Patient away. */
		examRoomNurse_id = examRoomNurseID[examRoom_id];		/* record the Nurse who takes Patient away from the Exam Room for the first time. */
		xrayRoom_id = xrayRoomID[examRoom_id];		/* the Xray Room he needs to go to. */
		Release(examRoomLockArray[examRoom_id]);	
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FREE;
		Release(examRoomLock);
		
		Acquire(xrayWaitingLock[xrayRoom_id]);		
		if (xrayState[xrayRoom_id] == X_BUSY || xrayState[xrayRoom_id] == X_FINISH) {		/* the Xray Technician is BUSY or FINISH. then the Patient gets in line. */
			++xrayWaitList[xrayRoom_id];
			Wait(xrayWaitingLock[xrayRoom_id], xrayWaitingCV[xrayRoom_id]);
		}else {
			xrayState[xrayRoom_id] = X_BUSY;
		}
		Release(xrayWaitingLock[xrayRoom_id]);		
		/* the Patient enters the Xray Room. */
		Acquire(xrayInteractLock[xrayRoom_id]);
		xrayExamSheet[xrayRoom_id] = myExamSheet;
		xrayPatientID[xrayRoom_id] = patient_ID;
		xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);/* the Patient gives the Exam Sheet to the Xray Technician. */
		
		/* Start to take xrays. */
		i = 0;
		while (i < myExamSheet->numberOfXray) {
			Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			if (i == 0) {
				print("Adult Patient [%d] gets on the table.\n", patient_ID);
			}
			print("Adult Patient [%d] has been asked to take an Xray.\n", patient_ID);
			Signal(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			Signal(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			i++;
		}		/* Xray Technician finishes taking xrays for the Patient. */
		Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
		print("Adult Patient [%d] waits for a Nurse to escort him/her to Exam room.\n", patient_ID);		/*there's a problem with the output line. */
		Acquire(xrayCheckingLock);
		xrayState[xrayRoom_id] = X_FINISH;
		Release(xrayCheckingLock);
		Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);	/* the Patient is waiting for a Nurse to take him/her back to the Exam Room. */
		
		/* a second Nurse comes to escort Patient away back to Exam Room. */
		examRoomSecondTime_id = examRoomSecondTimeID[xrayRoom_id];
		Release(xrayInteractLock[xrayRoom_id]);
		Acquire(examRoomLockArray[examRoomSecondTime_id]);
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		Acquire(examRoomLock);
		examRoomState[examRoomSecondTime_id] = E_READY;
		examRoomTask[examRoomSecondTime_id] = E_SECOND;
		Release(examRoomLock);
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
		
		Signal(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		print("Adult Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, examRoomDoctorID[examRoomSecondTime_id]);
		Signal(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		Acquire(examRoomLock);
		examRoomState[examRoomSecondTime_id] = E_FINISH;
		myExamSheet->goToCashier = 1;
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		Release(examRoomLock);
		/* the Patient waits for a third Nurse to take him/her to the Cashier. */
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);
		Release(examRoomLockArray[examRoomSecondTime_id]);	
		Acquire(examRoomLock);
		examRoomState[examRoomSecondTime_id] = E_FREE;
		Release(examRoomLock);
	}
	else if (1 == myExamSheet->shot) {	/* the Patient needs a shot. */
		print("Adult Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, examRoomDoctorID[examRoom_id]);
		print("Adult Patient [%d] has been informed by Doctor [%d] that he will be administered a shot.\n", patient_ID, examRoomDoctorID[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		Release(examRoomLock);
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		
		print("Adult Patient [%d] says, \"Yes I am ready for the shot\".\n", patient_ID);
		Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		/* the Patient waits for the Nurse to take him/her to the Cashier. */
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		myExamSheet->goToCashier = 1;
		examRoomExamSheet[examRoom_id] = myExamSheet;
		Release(examRoomLock);		
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		Release(examRoomLockArray[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FREE;
		Release(examRoomLock);
	}
	else {	/* the Patient has no problem and can go to the Cashier directly. */
		print("Adult Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, examRoomDoctorID[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		myExamSheet->goToCashier = 1;
		examRoomExamSheet[examRoom_id] = myExamSheet;
		Release(examRoomLock);
		/* the Patient waits for a second Nurse to take him/her to the Cashier. */
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		Release(examRoomLockArray[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FREE;
		Release(examRoomLock);
	}
	
	/* Patient is escorted to the Cashier. */
	Acquire(cashierWaitingLock);		
	if (cashierState == C_BUSY) {		
		++cashierWaitList;
		print("Adult Patient [%d] enters the queue for Cashier.\n", patient_ID);
		Wait(cashierWaitingLock, cashierWaitingCV);
	}else {
		cashierState = C_BUSY;
	}
	Release(cashierWaitingLock);
	
	/* interact with the Cashier. */
	Acquire(cashierInteractLock);
	cashierExamSheet = myExamSheet;
	print("Adult Patient [%d] reaches the Cashier.\n", patient_ID);
	print("Adult Patient [%d] hands over his examination sheet to the Cashier.\n", patient_ID);
	Signal(cashierInteractLock, cashierInteractCV);			
	Wait(cashierInteractLock, cashierInteractCV);	
	print("Adult Patient [%d] pays the Cashier $%d.\n", patient_ID, cashierExamSheet->price);
	Signal(cashierInteractLock, cashierInteractCV);			
	Wait(cashierInteractLock, cashierInteractCV);
	print("Adult Patient [%d] receives a receipt from the Cashier.\n", patient_ID);
	Signal(cashierInteractLock, cashierInteractCV);			
	Wait(cashierInteractLock, cashierInteractCV);
	
	/* Patient finishes everything and leaves. */
	print("Adult Patient [%d] leaves the doctor's office.\n", patient_ID);
	Signal(cashierInteractLock, cashierInteractCV);			
	Release(cashierInteractLock);
	--remainPatientCount;	/* one Patient leaves the Doctor's office. */
	Exit(0);
}

/* Parent */
void Parent() {
	int index = indexParent++;
	ExamSheet* myExamSheet;		/* the Patient declares a new Exam Sheet for himself. */
	char patient_id[10];
	char patient_name[20];
	int patient_ID = index;
	int patient_age = rand() % 14 + 1;
	int i = 0, symptomID = -1;
	int nurse_id = -1;
	int examRoom_id = -1;
	int examRoomSecondTime_id = -1;
	int examRoomNurse_id = -1;
	int xrayRoom_id = -1;
	
	Acquire(parentWaitChildLock[patient_ID]);
	if (childEnter[patient_ID] == 0) { /* if child has not entered, parent has to wait for child. */ 
		Wait(parentWaitChildLock[patient_ID], parentWaitChildCV[patient_ID]);
		Release(parentWaitChildLock[patient_ID]);
	}else {
		Release(parentWaitChildLock[patient_ID]);
	}

	/* the Patient gets in the line for the first time to get a form. */
	Acquire(WRLock);		/* the Patient tries to enter the Waiting Room and stay in line. */
	if (waitingRoomNurseState == W_BUSY) {		/* the Waiting Room Nurse is busy. then the Patient gets in line. */
		patientWaitingCount++;
		Wait(WRLock, patientWaitingCV);
	}else {
		waitingRoomNurseState = W_BUSY;
	}
	Release(WRLock);		/* the Patient finishes waiting and gets ready to interact with the Waiting Room Nurse. */
	
	Acquire(WRInteractLock);
	/*need to tell the Waiting Room Nurse what to do -- firstly get a form. */
	WRNurseTask = W_GETFORM;
	wrnPatientID = patient_ID;
	wrnPatientAge = patient_age;
	Signal(WRInteractLock, WRInteractCV);		/* tell the Waiting Room Nurse that he comes. */
	Wait(WRInteractLock, WRInteractCV);		/* wait for the Waiting Room Nurse to make a new sheet. */
	print("[Parent of child patient] gets the form from the Waiting Room Nurse.\n");
	myExamSheet = wrnExamSheet;		/* get the Exam Sheet. */
	Signal(WRInteractLock, WRInteractCV);		/* tell the Waiting Room Nurse that he gets the sheet. */
	Release(WRInteractLock);
	
	/* Patient goes away and fills the form. */
	myExamSheet->patientID = patient_ID;		/* write the Patient id, his name, on the sheet. */
	myExamSheet->age = patient_age;		/* write the age of the Patient on the Exam Sheet. Adult Patient age should be larger than 15. */
	sprint(patient_id, "%d ", patient_ID);
	strcpy(patient_name, "Patient_");
	strcat(patient_name, patient_id);
	myExamSheet->name = patient_name;		/* write the name string on the Exam Sheet struct for print. */
	
	/* the Patient gets back to the line to submit the form. */
	Acquire(WRLock);		/* the Patient tries to enter the Waiting Room and stay in line. */
	if (waitingRoomNurseState == W_BUSY) {		/* the Waiting Room Nurse is busy. then the Patient gets in line. */
		patientWaitingCount++;
		Wait(WRLock, patientWaitingCV);
	}else {
		waitingRoomNurseState = W_BUSY;
	}
	Release(WRLock);		/* the Patient finishes waiting and gets ready to interact with the Waiting Room Nurse. */
	
	Acquire(WRInteractLock);
	/* need to tell the Waiting Room Nurse what to do -- secondly turn over a form. */
	WRNurseTask = W_GIVEFORM;
	print("[Parent of child patient] submits the filled form to the Waiting Room Nurse.\n");
	wrnExamSheet = myExamSheet;
	numOfPatientsPassed++;	/* one Patient passed the interaction with the Waiting Room Nurse. */
	Signal(WRInteractLock, WRInteractCV);		/* turn over the filled form. */
	
	/* Patient waits for a Nurse in line. */
	Acquire(patientWaitNurseLock);
	patientWaitNurseCount++;
	Release(WRInteractLock);		/* Patient leaves the Waiting Room Nurse. */
	Wait(patientWaitNurseLock, patientWaitNurseCV);		/* Patient waits for a Nurse come to escort him. */
	/*******************************/
	/* Parent and Child interaction */
	
	print("Parent [%d] asks Child Patient [%d] to follow them.\n", patient_ID, patient_ID);
	
	/*******************************/
	if(nurseWaitPatientCount > 0)
	{	/* before leaving, wake up the next Nurse in line to get prepared for escorting the next Patient in line away. */
		Signal(patientWaitNurseLock, nurseWaitPatientCV);
		isNurseInteract = N_BUSY;
	}
	else
	{
		isNurseInteract = N_FREE;
	}
	
	/* escorted away by the Nurse. */
	nurse_id = wrnNurseIndex;		/* Patient knows which Nurse escorts him/her away. */
	Release(patientWaitNurseLock);
	
	/* Patient tries to acquire the lock for Nurse. */
	Acquire(nurseLock[nurse_id]);
	patientID[nurse_id] = patient_ID;
	nurseExamSheet[nurse_id] = myExamSheet;
	Signal(nurseLock[nurse_id], nurseCV[nurse_id]);
	Wait(nurseLock[nurse_id], nurseCV[nurse_id]);		/* waiting for the Nurse to get his Exam Sheet. */
	myExamSheet = nurseExamSheet[nurse_id];		/* get information back from the Nurse and know symptoms. */
	examRoom_id = myExamSheet->examRoomID;
	Release(nurseLock[nurse_id]);
	
	/* Patient enters the Exam Room. */
	Acquire(examRoomLockArray[examRoom_id]);
	print("[Parent] [%d] is following Nurse [%d] to Examination Room [%d].\n", patient_ID, nurse_id, examRoom_id);
	print("[Parent] [%d] has arrived at Examination Room [%d].\n", patient_ID, examRoom_id);
	Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	symptomID = rand()%3;
	if(symptomID == 0)
		myExamSheet->symptom = "pain";
	else if(symptomID == 1)
		myExamSheet->symptom = "nausea";
	else
		myExamSheet->symptom = "hear alien voices";
	print("Parent [%d] says, \"His symptoms are [%s]\".\n", patient_ID, myExamSheet->symptom);
	examRoomExamSheet[examRoom_id] = myExamSheet;
	Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	Acquire(examRoomLock);
	examRoomState[examRoom_id] = E_READY;
	examRoomTask[examRoom_id] = E_FIRST;
	Release(examRoomLock);
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
	
	/* turn over the exam sheet to the Doctor. */
	examRoomPatientID[examRoom_id ] = index;
	Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		
	Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);		
	myExamSheet = examRoomExamSheet[examRoom_id];		/* Doctor finishes the first time Examination. */
	
	if (1 == myExamSheet->xray) {	/* the Patient needs to take an Xray. */
		/*******************************/
		/* Parent and Child interaction */
		Acquire(parentChildLock[patient_ID]);
		parentChildState[patient_ID] = P_EXAMXRAY;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoom_id];
		Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Release(parentChildLock[patient_ID]);
		
		/*******************************/
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		Release(examRoomLock);
		print("Parent [%d] waits for a Nurse to escort them to the Xray room.\n", patient_ID);
		/*******************************/
		/* Parent and Child interaction */
		
		print("Parent [%d] asks Child Patient [%d] to follow them.\n", patient_ID, patient_ID);
		
		/*******************************/
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);	/* the Patient waits for a Nurse to escort him/her to Xay Room. */

		/* a Nurse comes to escort Patient away. */
		examRoomNurse_id = examRoomNurseID[examRoom_id];		/* record the Nurse who takes Patient away from the Exam Room for the first time. */
		xrayRoom_id = xrayRoomID[examRoom_id];		/* the Xray Room he needs to go to. */
		Release(examRoomLockArray[examRoom_id]);	
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FREE;
		Release(examRoomLock);
		
		Acquire(xrayWaitingLock[xrayRoom_id]);		
		if (xrayState[xrayRoom_id] == X_BUSY || xrayState[xrayRoom_id] == X_FINISH) {		/* the Xray Technician is BUSY or FINISH. then the Patient gets in line. */
			++xrayWaitList[xrayRoom_id];
			Wait(xrayWaitingLock[xrayRoom_id], xrayWaitingCV[xrayRoom_id]);
		}else {
			xrayState[xrayRoom_id] = X_BUSY;
		}
		Release(xrayWaitingLock[xrayRoom_id]);		
		/* the Patient enters the Xray Room. */
		Acquire(xrayInteractLock[xrayRoom_id]);
		xrayExamSheet[xrayRoom_id] = myExamSheet;
		xrayPatientID[xrayRoom_id] = patient_ID;
		xrayInteractCV[xrayRoom_id]->Signal(xrayInteractLock[xrayRoom_id]);/* the Patient gives the Exam Sheet to the Xray Technician. */
		
		/* Start to take xrays. */
		i = 0;
		while (i < myExamSheet->numberOfXray) {
			Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			if (i == 0) {
				/***************************/
				/* Parent and Child interaction. */
				Acquire(parentChildLock[patient_ID]);
				parentChildState[patient_ID] = P_TABLE;
				Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
				Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
				Release(parentChildLock[patient_ID]);
			}else {
				/***************************/
				/* Parent and Child interaction. */
				Acquire(parentChildLock[patient_ID]);
				parentChildState[patient_ID] = P_MOVE;
				Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
				Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
				Release(parentChildLock[patient_ID]);
			}
			/***************************/
			/* Parent and Child interaction */
			
			Acquire(parentChildLock[patient_ID]);
			parentChildState[patient_ID] = P_XRAY;
			Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
			Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
			Release(parentChildLock[patient_ID]);
			
			/*********************************/
			Signal(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			Signal(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
			i++;
		}		/* Xray Technician finishes taking xrays for the Patient. */
		Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);
		print("Parent [%d] waits for a Nurse to escort them to Exam room.\n", patient_ID);		/*there's a problem with the output line. */
		Acquire(xrayCheckingLock);
		xrayState[xrayRoom_id] = X_FINISH;
		Release(xrayCheckingLock);
		Wait(xrayInteractLock[xrayRoom_id], xrayInteractCV[xrayRoom_id]);	/* the Patient is waiting for a Nurse to take him/her back to the Exam Room. */
		
		/* a second Nurse comes to escort Patient away back to Exam Room. */
		examRoomSecondTime_id = examRoomSecondTimeID[xrayRoom_id];
		Release(xrayInteractLock[xrayRoom_id]);
		Acquire(examRoomLockArray[examRoomSecondTime_id]);
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		Acquire(examRoomLock);
		examRoomState[examRoomSecondTime_id] = E_READY;
		examRoomTask[examRoomSecondTime_id] = E_SECOND;
		Release(examRoomLock);
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	/* get into a Exam Room and wait for a Doctor, together with a Nurse. */
		
		Signal(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		/***************************/
		/* Parent and Child interaction. */
		
		Acquire(parentChildLock[patient_ID]);
		parentChildState[patient_ID] = P_EXAMDIAGNOSED;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoomSecondTime_id];
		Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Release(parentChildLock[patient_ID]);
		
		/********************************/
		Signal(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);	
		Acquire(examRoomLock);
		examRoomState[examRoomSecondTime_id] = E_FINISH;
		myExamSheet->goToCashier = 1;
		examRoomExamSheet[examRoomSecondTime_id] = myExamSheet;
		Release(examRoomLock);
		/* the Patient waits for a third Nurse to take him/her to the Cashier. */
		Wait(examRoomLockArray[examRoomSecondTime_id], examRoomCVArray[examRoomSecondTime_id]);
		Release(examRoomLockArray[examRoomSecondTime_id]);	
		Acquire(examRoomLock);
		examRoomState[examRoomSecondTime_id] = E_FREE;
		Release(examRoomLock);
	}
	else if (1 == myExamSheet->shot) {	
		/***************************/
		/* Parent and Child interaction */
		Acquire(parentChildLock[patient_ID]);
		parentChildState[patient_ID] = P_EXAMSHOT;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoom_id];
		Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Release(parentChildLock[patient_ID]);
		/********************************/
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		Release(examRoomLock);
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		
		print("Parent [%d] says, \"He is ready for the shot\".\n", patient_ID);
		print("Child patient [%d] has been given a shot.\n", patient_ID);
		Signal(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		/* the Patient waits for the Nurse to take him/her to the Cashier. */
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		myExamSheet->goToCashier = 1;
		examRoomExamSheet[examRoom_id] = myExamSheet;
		Release(examRoomLock);		
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		Release(examRoomLockArray[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FREE;
		Release(examRoomLock);
	}
	else {	/* the Patient has no problem and can go to the Cashier directly. */
		/***************************/
		/* Parent and Child interaction */
		Acquire(parentChildLock[patient_ID]);
		parentChildState[patient_ID] = P_EXAMDIAGNOSED;
		parentTellChildDocID[patient_ID] = examRoomDoctorID[examRoom_id];
		Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Release(parentChildLock[patient_ID]);
		/********************************/
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FINISH;
		myExamSheet->goToCashier = 1;
		examRoomExamSheet[examRoom_id] = myExamSheet;
		Release(examRoomLock);
		/* the Patient waits for a second Nurse to take him/her to the Cashier. */
		Wait(examRoomLockArray[examRoom_id], examRoomCVArray[examRoom_id]);
		Release(examRoomLockArray[examRoom_id]);
		Acquire(examRoomLock);
		examRoomState[examRoom_id] = E_FREE;
		Release(examRoomLock);
	}
	
	/* Patient is escorted to the Cashier. */
	Acquire(cashierWaitingLock);		
	if (cashierState == C_BUSY) {		
		++cashierWaitList;
		print("Parent [%d] enters the queue for Cashier.\n", patient_ID);
		Wait(cashierWaitingLock, cashierWaitingCV);
	}else {
		cashierState = C_BUSY;
	}
	Release(cashierWaitingLock);
	
	/* interact with the Cashier. */
	Acquire(cashierInteractLock);
	cashierExamSheet = myExamSheet;
	print("Parent [%d] reaches the Cashier.\n", patient_ID);
	print("Parent [%d] hands over Child Patient [%d]'s examination sheet to the Cashier.\n", patient_ID, patient_ID);
	Signal(cashierInteractLock, cashierInteractCV);			
	Wait(cashierInteractLock, cashierInteractCV);	
	print("Parent [%d] pays the Cashier $%d.\n", patient_ID, cashierExamSheet->price);
	Signal(cashierInteractLock, cashierInteractCV);			
	Wait(cashierInteractLock, cashierInteractCV);
	print("Parent [%d] receives a receipt for Child Patient [%d] from the Cashier.\n", patient_ID, patient_ID);
	Signal(cashierInteractLock, cashierInteractCV);			
	Wait(cashierInteractLock, cashierInteractCV);
	
	/* Patient finishes everything and leaves. */
	print("Parent [%d] leaves the doctor's office with Child Patient [%d].\n", patient_ID, patient_ID);
	Signal(cashierInteractLock, cashierInteractCV);			
	Release(cashierInteractLock);
	/***************************/
	/* Parent and Child interaction. */
	
	Acquire(parentChildLock[patient_ID]);
	leave[patient_ID] = 1;
	Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
	Release(parentChildLock[patient_ID]);
	
	/*parent leaves the doctor's office with the child */
	/*****************************************/
	--remainPatientCount;	/* a Patient leaves the Docotr's office. */
	Exit(0);
}

/* Child Patient */
void Child() {
	int index = indexChild++;
	int doctor_id;
	int patient_ID = index;
	
	Acquire(parentWaitChildLock[patient_ID]);
	print("Child Patient [%d] has entered the Doctor's Office Waiting Room with Parent [%d].\n", patient_ID, patient_ID);
	childEnter[patient_ID] = 1;
	Signal(parentWaitChildLock[patient_ID], parentWaitChildCV[patient_ID]);
	Acquire(parentChildLock[patient_ID]);
	Release(parentWaitChildLock[patient_ID]);

	/*child waits till his/her parent tells him/her what to do. */
	Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);
	while (0 == leave[patient_ID]) {
		if (parentChildState[patient_ID] == P_EXAMXRAY) {		/*needs an Xray. */
			doctor_id = parentTellChildDocID[patient_ID];
			print("Child Patient [%d] has been informed by Doctor [%d] that he needs an Xray.\n", patient_ID, doctor_id);
		}
		else if (parentChildState[patient_ID] == P_EXAMSHOT) {		/*needs a shot. */
			doctor_id = parentTellChildDocID[patient_ID];
			print("Child Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, doctor_id);
			print("Child Patient [%d] has been informed by Doctor [%d] that he will be administered a shot.\n", patient_ID, doctor_id);
		}
		else if (parentChildState[patient_ID] == P_EXAMDIAGNOSED) {		/*finish diagnosing. */
			doctor_id = parentTellChildDocID[patient_ID];
			print("Child Patient [%d] has been diagnosed by Doctor [%d].\n", patient_ID, doctor_id);
		}
		else if (parentChildState[patient_ID] == P_TABLE) {		/*get on the table. */
			print("Child Patient [%d] gets on the table.\n", patient_ID);
		}
		else if (parentChildState[patient_ID] == P_XRAY) {		/*has been asked to take an xray. */
			print("Child Patient [%d] has been asked to take an Xray.\n", patient_ID);
		}
		else if (parentChildState[patient_ID] == P_MOVE) {		/*has been asked to take an xray. */
			print("Child Patienht [%d] moves for the next Xray.\n", patient_ID);
		}
		Signal(parentChildLock[patient_ID], parentChildCV[patient_ID]);
		Wait(parentChildLock[patient_ID], parentChildCV[patient_ID]);		/*waits for parent's next instruction. */
	}	/*end of while. */
	Release(parentChildLock[patient_ID]);		/*child leaves with his/her parent. */
	Exit(0);
}

/*
 * Problem2
 *	The entrance of the Assignment 1 Problem 2.
 *	Display the command menu for different test cases.
 */
void Problem2(void)
{
	int i = 0;
	numberOfDoctors = 3;
	numberOfNurses = 5;
	numberOfXrays = 2;
	numberOfExamRooms = 5;
	numberOfAdults = 30;
	numberOfWRNs = numberOfCashiers = 1;
	numberOfChildren = numberOfParents = 30;
	remainNurseCount = numberOfNurses;
	remainPatientCount = numberOfPatients = numberOfAdults + numberOfChildren; 
	
	/* Initialize the Locks, Condition Variables, Monitor Variables */
	/* and the initial state of each entity. */
	Init();
	
	/* Fork Doctors. */
	for(i = 0; i < numberOfDoctors; ++i){
		Fork(Doctor);
	}
	
	/* Fork Nurses. */
	for(i = 0; i < numberOfNurses; ++i){
		Fork(Nurse);
	}
	
	/* Fork Xray Technicians. */
	for(i = 0; i < numberOfXrays; ++i){
		Fork(XrayTechnician);
	}
	
	/* Fork Waiting Room Nurses. */
	for(i = 0; i < numberOfWRNs; ++i){
		Fork(WaitingRoomNurse);
	}
	
	/* Fork Cashiers. */
	for(i = 0; i < numberOfCashiers; ++i){
		Fork(Cashier);
	}
	
	/* Fork Adult Patients. */
	for(i = 0; i < numberOfAdults; ++i){
		Fork(Patient);
	}
	
	/* Fork Child Patients. */
	for(i = numberOfAdults; i < numberOfChildren + numberOfAdults; ++i){
		Fork(Child);
	}
	
	/* Fork Parents. */
	for(i = numberOfAdults; i < numberOfParents + numberOfAdults; ++i){
		Fork(Parent);
	}
}
