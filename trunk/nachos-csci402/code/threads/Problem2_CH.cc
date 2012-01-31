// Author: Chen Hao
// Date: 2012-01-26

#include "synch.h"
#include <ctime>

#define MAX_DOCTOR 3
#define MAX_EXAM_ROOM 3
#define MAX_NURSE 5
#define MAX_XRAY 2
#define XRAY_IMAGE 3
#define MAX_NURSE 5
#define BUSY 0
#define FREE 1


int numberOfDoctors;
int numberOfNurses;
int numberOfWRNs;
int numberOfCashiers;
int numberOfXrays;
int numberOfAdults;
int numberOfChildren;

struct ExamSheet{  // examination sheet

    int patientID;
    int age;
	char* name;
	int examRoomID;
	bool shot;
	bool xray;
	char* xrayImage[XRAY_IMAGE];  // 'nothing', 'break', 'compound fracture'.
    float price;
	char* result;  // the result of examination.
	bool goToCashier;
};


struct Pocket{
    
	int patientID;
	int examRoomID;
	int state;
};


Lock* cabinetLock;
Condition* cabinetCV;




Lock* examRoomLock; // the lock for all of the examination rooms
Condition* examRoomCV;
ExamSheet* examRoomExamSheet[MAX_NURSE];
Lock* examRoomLockArray[MAX_NURSE];
Condition* examRoomCVArray[MAX_NURSE];
int examRoomDoctorID[MAX_NURSE];
int examRoomNurseID[MAX_NURSE];
enum ExamRoom_State{
    E_BUSY, E_FREE, E_READY, E_FINISH
};
enum ExamRoom_Task{
	E_FIRST, E_SECOND
};
ExamRoom_State examRoomState[MAX_NURSE]; //monitor variable: the state of each examRoom initial value: free.
examRoom_Task examRoomTask[MAX_NURSE];
int xrayRoomID[MAX_NURSE]; //the xray room id that the patient will go to wait


// Doctor:
int doctorWaitingCount[MAX_DOCTOR];
Lock* doctorWaitingLock[MAX_DOCTOR];
Lock* doctorInteractLock[MAX_DOCTOR];
Lock* allDoctorLock;
Condition* doctorWaitingCV[MAX_DOCTOR];
Condition* cashierInteractCV[MAX_DOCTOR];
enum Doctor_State{
	D_BUSY, D_FREE, D_FININSH
};
enum Doctor_Task{
	T_FIRST, T_SECOND
};
Doctor_State doctorState[MAX_DOCTOR];
Doctor_Task doctorTask[MAX_DOCTOR];
ExamSheet* examSheetDoctor[MAX_DOCTOR];	// the current Examination Sheet from the Patient in Doctor().
//End of doctor





Lock* patientWaitNurseLock;   //the lock of ready patient wait for the nurse to escort them
Condition* patientWaitNurseCV;  // the condition variable for the monitor.
Condition* nurseWaitPatientCV; // the condition variable for the nurse to wait.
//Condition* nurseWaitExamSheet; // the condition variable for the nurse to wait that the patient give the examsheet to nurse.
int patientWaitNurseCount;   //monitor variable initial value: 0
int nurseWaitPatientCount;  // initial value: 0. the number of nurses who are going into the monitor, 
                     //and are waiting for the patient to signal when the patient leaves the waiting room
int wrnNurseIndex;    //the id of the nurse who are interacting  with the patient in waitingRoom					 
bool isNurseInteracting;  //to indicate that if one nurse is interacting with a patient in the waiting room. initial value: 0					 
int nurseWaitPatient_count; // to indicate how many nurses waiting in the nurseWaitPatientCV


Lock* nurseLock[MAX_NURSE];
ExamSheet* nurseExamSheet[MAX_NURSE]; //monitor variable: To get the examSheet of the patient. initial in the patient thread
Condition* nurseCV[MAX_NURSE];
int patientID[MAX_NURSE];
int examRoomID[MAX_NURSE];

Lock* nurseWrnLock; // lock for nurse interact with Wrn
Condition* nurseWrnCV;
//int nurseTakeSheetID;
List* nurseTakeSheetID;
ExamSheet* wrnExamSheet; // monitor variable: to transfer from wrn to nurse
int nurseWaitWrnCount; // the number of nurses waiting for Wrn;
int nurseWaitPatientCount; //the number of nurses waiting for the patient;
int patientWaitNurseCount; // the number of patient waiting for the nurse;
int nextActionForNurse; //the state to indicate a nurse is to take pick up a patient or not


int xrayWaitingCount[MAX_XRAY];
int xrayPatientID[MAX_XRAY];
Lock* xrayWaitingLock[MAX_XRAY];
Lock* xrayInteractLock[MAX_XRAY];	// there could be MAX_XRAY (2) Xray Technicians.
int examRoomSecondTimeID[MAX_XRAY];
Condition* xrayWaitingCV[MAX_XRAY];
Condition* xrayInteractCV[MAX_XRAY];
enum Xray_State{
	X_BUSY, X_FREE, X_FINISH
};
Xray_State xrayState[MAX_XRAY];
ExamSheet* xrayExamSheet[MAX_XRAY];	// the current Examination Sheet from the Patient in XrayTechnician().
Lock* xrayCheckingLock;



Lock* doctorLock[MAX_DOCTOR];


Pocket* allPockets[100];
Lock* pocketsLock;


void Nurse(unsigned int index)
{
    ExamSheet* patientExamSheet;
    while(true)
	{
	    fprintf(stdout, "Nurse %u tries to check if there is examRoom available.\n", index);
		examRoomLock->Acquire();    //Go to see if there is any doctor/exminationRoom available
		int i = 0;
	    for(; i < numberOfNurses; ++i)  // check to see which room is avaiable
            if(examRoomState[i] == D_FREE)
            {
			    break;
			}
        if(i == numberOfDoctors) // no room is availble at this time
		{
		    fprintf(stdout, "Nurse %u doesn't find any available examinationRoom, so, he/she releases the allExamRoomLock");
		    examRoomLock->Release();
		}
		else{
		    
			examRoomState[i] = D_BUSY;
			fprintf(stdout, "Nurse %u tries to get the lock of examRoom %d.\n", index,i);
			//examRoomLockArray[i]->Acquire();
			//fprintf(stdout, "Nurse %u releases the allExamRoomLock");
			examRoomLock->Release();
			//fprintf(stdout, "Nurse %u tries to tries to acquire nurseWrnLock.\n", index);
			nurseWrnLock->Acquire();
			//int *tempNurseIndex;
			//tempNurseIndex = &index;
			nurseTakeSheetID->Append((void *)&index);
			//nurseTakeSheetID = index;
			nurseWaitWrnCount++;
			fprintf(stdout, "Nurse[%d] tells Waiting Room Nurse to give a new examination sheet.", index);
			nurseWrnCV->Wait(nurseWrnLock);
			nurseWaitWrnCount--;
			if(nextActionForNurse == 1)
			{
			    //fprintf(stdout, "nurse %u is informed by Wrn to go to pick up a patient", index);
			    //fprintf(stdout, "nurse %u gets examSheet from Wrn", index);
			    patientExamSheet = wrnExamSheet;
				
				//nurseWrnCV->Signal(nurseWrnCV);
				//nurseWaitPatientCount++;
				
				nurseWrnLock->Release();
				//fprintf(stdout, "nurse %u tries to get patientWaitNurseLock to take a patient", index);
				patientWaitNurseLock->Acquire();
				nurseWaitPatientCount++;
				if(isNurseInteracting == BUSY)
				{
				    
			        //fprintf(stdout, "nurse %u goes to wait to take a patient, because another nurse is interacting", index);
				    //nurseWaitPatient_count++;
					nurseWaitPatientCV->Wait(patientWaitNurseLock);
				}
				isNurseInteracting = BUSY;
				//nurseWaitPatient_count--;
				nurseWaitPatientCount--;
				patientWaitNurseCount--;
				patientWaitNurseCV->Signal(patientWaitNurseLock);
				
				//
				//
				wrnNurseIndex = index;
				nurseLock[index]->Acquire();
				patientWaitNurseLock->Release();
				nurseCV[index]->Wait(nurseLock[index]);
				fprintf(stdout, "Nurse [%d] escorts Adult Patient /Parent [%d] to the waiting room.", index, patientID[index]);
				fprintf(stdout, "nurse [%u] is escorting patient [%d] to examinationRoom", index, patientID[index]);
				//fprintf(stdout, "In the examinationRoom %d, nurse %d is examing patient %d", i,index,patientID[index]);
				
				int patient_ID = patientID[index];
				
				nurseExamSheet[index] = patientExamSheet;
				nurseExamSheet[index]->examRoomID = i;
				
				nurseCV[index]->Signal(nurseLock[index]);
				examRoomLockArray[i]->Acquire();
				examRoomTask[i] = E_FIRST;
                
				fprintf(stdout, "Nurse [%d] takes the temperature and blood pressure of Adult Patient /Parent [%d].", index, patientID[index]);
				fprintf(stdout, "Nurse [%d] asks Adult Patient /Parent [%d] “What Symptoms do you have?”", index, patientID[index]);
				fprintf(stdout, "Nurse [%d] writes all the information of Adult Patient /Parent [%d] in his examination sheet.", index, patientID[index]);
			    nurseLock[index]->Release();
				
			    examRoomCVArray[i]->Wait(examRoomLockArray[i]);	
				fprintf(stdout, "Nurse [%d] informs Doctor [%d] that Adult/Child Patient [%d] is waiting in the examination room %d.", index,examRoomDoctorID[i],patient_ID,i);
				fprintf(stdout, "Nurse [%d] hands over to the Doctor [%d]  the examination sheet of Adult/Child Patient [%d].", index,examRoomDoctorID[i],patient_ID);
				examRoomLockArray[i]->Release();
				
				//patientExamSheet->doctorID = i;
				
				//examRoomCVArray[i]->Wait();
				
				
				
			}
		    else
			{
			    //fprintf(stdout, "nurse %u tries to release examRoomLock %d and nurseWrnLock beacuse he/she doesn't need to go to take patient", index, i);
			    examRoomLock->Acquire();
				examRoomState[i] = FREE;
				examRoomLock->Release();
				//examRoomLockArray[i]->Release(); 
				//nurseWrnCV->Signal(nurseWrnCV);
				nurseWrnLock->Release();
			}
			
// end of task1

srand(time(0));
int r = rand()%100 + 50;
int loopTime = 0;
for(; loopTime != r; ++loopTime)
	currentThread->Yield();

			
//Task 2:
    /*
    pocketsLock->Acquire();
	Pocket* myPocket = new Pocket;
	loopTime = 0;
	for(; loopTime < 100; ++loopTime)
	    if(allPockets[loopTime]->state == FREE || allPockets[loopTime]->state == UNINITIAL)
		{
		    break;
		}
		
	if(allPockets[loopTime] == FREE)
	{
	    myPocket = allPockets[loopTime];
	}

    //suppose the examinationSheet is :
	
	int examRoomNumber = patientExamSheet->examRoomID;
	fprintf(stdout, "Nurse %u go to examinationRomm %d to pick up patient %d", index,examRoomNumber,patientExamSheet->patientID);
	examRoomLockArray[examRoomNumber]->Acquire();
	examRoomCVArray[examRoomNumber]->Signal();
	
	if(patientExamSheet->xRay == true)
	{
	    
	}
			
	*/

/*
allDoctorLock->Acquire();	
int doctor_index = 0;
for(; doctor_index < numberOfDoctors; ++doctor_index)
{
    if(doctorState[doctor_index] == D_FINISH)  //find that doctor_index is finish with one patient
	{
	    doctorState[doctor_index] = D_BUSY;
	    break;
	}
}	
allDoctorLock->Release();
if(doctor_index != numberOfDoctors)
{
    doctorInteractLock[doctor_index]->Acquire();   //go to pick up the patient who is with doctor_index
	dctorInteractCV[doctor_index]->Broadcast(doctorInteractLock[doctor_index]);
	doctorNurseIndex[doctor_index] = index;
	doctorInteractLock[doctor_index]->Release();
	nurseCV[index]->Wait(nurseLock[index]);
}
*/
examRoomLock->Acquire();   //To go to get a patient

int loopRoomID = 0;   //the id for examRoomID
for(; loopRoomID < numberOfNurses; ++loopRoomID)
{
    if(examRoomState[loopRoomID] == E_FINISH)
	{
	    examRoomState[loopRoomID] = E_BUSY;
		break;
	}
}
examRoomLock->Release();	 

if(loopRoomID != numberOfNurses)  //when there is no patient with the state of finish.
{
    examRoomLockArray[loopRoomID]->Acquire();
    examRoomNurseID[loopRoomID] = index;	
	examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);  //Signal a person in the examination Room
	patientExamSheet = examRoomExamSheet[loopRoomID];
	if(patientExamSheet->xray && !(patientExamSheet->goToCashier))
	{
	    
		int xrayLoop = 0;
		int index_xray;
		xrayCheckingLock->Acquire();
		for(; xrayLoop != numberOfXrays; ++xrayLoop)
		{
		    if(xrayState[xrayLoop] == X_FREE)
			{
			    xrayState[xrayLoop] = X_BUSY;
			    break;
			}
		}
		xrayCheckingLock->Release();
		if(xrayLoop != numberOfXrays)
		{
		    xrayRoomID[loopRoomID] = xrayLoop;
		}
		
		else
		{
		    srand(time(0));
			index_xray = rand()%2;
			xrayRoomID[loopRoomID] = index_xray;
		}
	}
	
	if(patientExamSheet->shot && !(patientExamSheet->goToCashier))
	{
	    fprintf(std, "Nurse [%d] goes to supply cabinet to give to take medicine for Adult Patient /Parent [%d].", index,patientExamSheet->patientID);
		cabinetLock->Acquire();
		//Do sth in Cabinet
		cabinetLock->Release();
		fprintf(std,"Nurse [%d] asks Adult Patient /Parent [%d] Whether you are ready for the shot?", index, patientExamSheet->patientID);
		fprintf(std,"Nurse [%d] tells Adult Patient /Parent [%d] Shot is over.", index, patientExamSheet->patientID);
		examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);
		examRoomCVArray[loopRoomID]->Wait(examRoomLockArray[loopRoomID]);
		fprintf(std,"Nurse [%d] escorts Adult Patient /Parent [%d] to Cashier.", index, patientExamSheet->patientID);
		examRoomCVArray[loopRoomID]->Signal(examRoomLockArray[loopRoomID]);
		//examRoomLockArray[loopRoomID]->Release();
		
	}
	
	
	
	examRoomLockArray[loopRoomID]->Release();
	//examRoomLock->Acquire();
	//examRoomState[loopRoomID] = E_FREE;
	//examRoomLock->Release();
	
}	

else{  //No Patient is in state of "FINIFSH"
    
}		

srand(time(0));
r = rand()%100 + 50;
for(loopTime = 0; loopTime != r; ++loopTime)
	currentThread->Yield();

			
//Task 3:
	
examRoomLock->Acquire();
int loopID = 0; //to search a free room in the examRoomLock
for(; loopID < numberOfNurses; ++loopID)
{
    if(examRoomState[loopID] == E_FREE)
	{
	    examRoomState[loopID] = E_BUSY;
		break;
	}
	
}

examRoomLock->Release();

if(loopID != numberOfNurses) // there is a free examRoom
{
    xrayCheckingLock->Acquire();
	int tempXrayID = 0;
	for(; tempXrayID < numberOfXrays; ++tempXrayID)
	{
	    if(xrayState[tempXrayID] == X_FINISH)
		{
		    xrayState[tempXrayID] = X_READY;//.................?????????
			break;
		}
	}
	xrayCheckingLock->Release();
	if(tempXrayID != numberOfXrays)  //one patient finishs in a xrayRoom
	{
	    xrayInteractLock[tempXrayID]->Acquire();
	    examRoomSecondTimeID[tempXrayID] = loopID;
	    xrayInteractLock[tempXrayID]->Broadcast(xrayInteractLock);
	    examRoomLockArray[loopID]->Acquire();
	    xrayInteracLock[tempXrayID]->Release();
	    examRoomCVArray[loopID]->Wait(examRoomLockArray[loopID]);
	    fprintf(stdout, "Nurse [%d] informs Doctor [%d] that Adult/Child Patient [%d] is waiting in the examination room %d.", index,examRoomDoctorID[loopID],patientID[index],loopID); //有问题
        fprintf(stdout, "Nurse [%d] hands over to the Doctor [%d]  the examination sheet of Adult/Child Patient [%d].", index,examRoomDoctorID[loopID],patientID[index]);//有问题
	    examRoomLockArray[loopID]->Release();
	}
}

srand(time(0));
r = rand()%100 + 50;
for(int loopTime = 0; loopTime != r; ++loopTime)
	currentThread->Yield();


//task 4:   !!!// go to make sure that the patient is go to cahier

examRoomLock->Acquire();
int loopID = 0; //to search a free room in the examRoomLock
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
	    examRoomLockArray[loopID]->Signal(examRoomLockArray[loopID]);
	    fprintf(stdout, "Nurse [%d] escorts Adult Patient /Parent [%d] to Cashier.", index, examRoomExamSheet[loopID]);
	    
	}
	examRoomLockArray[loopID]->Release();
	else
	{
	    examRoomLock->Acquire();
		examRoomState[loopID] = E_FINISH;
		examRoomLock->Release();
		
	}
}

    



//Task 5........


	
			
	
		}
		
        		
	}
}





