// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "bitmap.h"
#include <stdio.h>
#include <iostream>
#include "synch.h"

#define MAX_LOCK 5001
#define MAX_CV  5001

using namespace std;

// System call Lock.
struct KernelLock{
	Lock* lock;
	AddrSpace* addrSpace;
	bool isDeleted;
	bool isToBeDeleted;
};

// System call CV.
struct KernelCV{
	Condition* cv;
	AddrSpace* addrSpace;
	bool isDeleted;
	bool isToBeDeleted;
};

KernelLock* kernelLock[MAX_LOCK];
KernelCV* kernelCV[MAX_CV];

Lock* lockForLock = new Lock("lockForLock");
Lock* lockForCV = new Lock("lockForCV");
BitMap lockBM(MAX_LOCK);
BitMap cvBM(MAX_CV);

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

int CreateLock_Syscall(unsigned int vaddr, int len)
{
	// Kernel buffer to load the name of the Lock.
	char* buf = new char[len+1];
	
	// Check for the allocation of the kernel buffer.
	if(!buf){
		printf("%s", "Can't allocate kernel buffer in CreateLock.\n");
		return -1;
	}
	
	// Check for the copy in operation from vaddr to buf with len bytes..
	if(-1 == copyin(vaddr, len, buf)){
		printf("%s","Bad pointer passed to CreateLock.\n");
		delete[] buf;
		return -1;
	}
	
	lockForLock->Acquire();
	int index = lockBM.Find();
	if(-1 == index){	// whether there is a available Kernel Lock.
		printf("%s", "Can't find available Kernel Lock.\n");
		lockForLock->Release();
		return -1;
	}
	// Construct a new Kernel Lock with the name of buf, located in position 
	// index of the kernelLock table.
	Lock* lock = new Lock(buf);
	kernelLock[index] = new KernelLock;
	kernelLock[index]->lock = lock;
	kernelLock[index]->addrSpace = currentThread->space;
	kernelLock[index]->isDeleted = false;
	kernelLock[index]->isToBeDeleted = false;
	lockForLock->Release();
	
	// Return the position (the index) of the kernel Lock.
	delete[] buf;
	return index;
}

int DeleteLock_Syscall(int lockIndex)
{
	lockForLock->Acquire();
	// Check for the validation of the attempt to delete the lock.
	if(lockIndex < 0 || lockIndex >= MAX_LOCK){
		printf("%s", "The specified lock is not in the Kernel Lock table.\n");
		lockForLock->Release();
		return -1;
	}else if(NULL == kernelLock[lockIndex] || NULL == kernelLock[lockIndex]->lock){
		printf("%s", "The specified lock has already been deleted.\n");
		lockForLock->Release();
		return -1;
	}else if(kernelLock[lockIndex]->addrSpace != currentThread->space){
		printf("%s", "The specified lock is not owned by the process.\n");
		lockForLock->Release();
		return -1;
	}
	
	// Check the status of the kernel lock.
	if(kernelLock[lockIndex]->isToBeDeleted){
		// Here, we can run the delete operation.
		delete kernelLock[lockIndex]->lock;
		lockBM.Clear(lockIndex);	// clear the BitMap bit.
		kernelLock[lockIndex]->addrSpace = NULL;
		kernelLock[lockIndex]->isDeleted = true;
		kernelLock[lockIndex]->isToBeDeleted = false;
	}else{
		printf("%s", "The kernel lock is currently in use.\n");
		lockForLock->Release();
		return -1;
	}
	
	lockForLock->Release();
	// Delete successed.
	return 0;
}

int CreateCondition_Syscall(unsigned int vaddr, int len)
{
	// Kernel buffer to load the name of the CV.
	char* buf = new char[len+1];
	
	// Check for the allocation of the kernel buffer.	
	if(!buf){
		printf("%s", "Can't allocate kernel buffer in CreateCondition.\n");
		return -1;
	}
	
	// Check for the copy in operation from vaddr to buf with len bytes..
	if(-1 == copyin(vaddr, len, buf)){
		printf("%s","Bad pointer passed to CreateCondition.\n");
		delete[] buf;
		return -1;
	}
	
	lockForCV->Acquire();
	int index = cvBM.Find();
	if(-1 == index){ // whether there is a available Kernel CV.
		printf("%s", "Can't find available Kernel Condition Variable.\n");
		lockForCV->Release();
		return -1;
	}
	// Construct a new Kernel CV with the name of buf, located in position 
	// index of the kernelCV table.
	Condition* cv = new Condition(buf);
	kernelCV[index] = new KernelCV;
	kernelCV[index]->cv = cv;
	kernelCV[index]->addrSpace = currentThread->space;
	kernelCV[index]->isDeleted = false;
	kernelCV[index]->isToBeDeleted = false;
	lockForCV->Release();
	
	// Return the position (the index) of the kernel CV.
	delete[] buf;
	return index;
}

int DeleteCondition_Syscall(int cvIndex)
{
	lockForCV->Acquire();
	// Check for the validation of the attempt to delete the cv.
	if(cvIndex < 0 || cvIndex >= MAX_CV){
		printf("%s", "The specified cv is not in the Kernel CV table.\n");
		lockForCV->Release();
		return -1;
	}else if(NULL == kernelCV[cvIndex] || NULL == kernelCV[cvIndex]->cv){
		printf("%s", "The specified cv has already been deleted.\n");
		lockForCV->Release();
		return -1;
	}else if(kernelCV[cvIndex]->addrSpace != currentThread->space){
		printf("%s", "The specified cv is not owned by the process.\n");
		lockForCV->Release();
		return -1;
	}
	
	// Check the status of the kernel cv.
	if(kernelCV[cvIndex]->isToBeDeleted){
		// Here, we can run the delete operation.
		delete kernelCV[cvIndex]->cv;
		cvBM.Clear(cvIndex);	// clear the BitMap bit.
		kernelCV[cvIndex]->addrSpace = NULL;
		kernelCV[cvIndex]->isDeleted = true;
		kernelCV[cvIndex]->isToBeDeleted = false;
	}else{
		printf("%s", "The kernel cv is currently in use.\n");
		lockForCV->Release();
		return -1;
	}
	
	lockForCV->Release();
	// Delete successed.
	return 0;
}

int Acquire_Syscall(int lockId){
	lockForLock->Acquire();
	if (lockId < 0 || lockId > MAX_LOCK) {
		//check if the lock id is within the range
		printf("Lock index is out of range.\n");
		lockForLock->Release();
		return -1;
	}else if (kernelLock[lockId] == NULL || kernelLock[lockId]->lock == NULL || kernelLock[lockId]->isDeleted) {
		//check if the lock id is deleted
		printf("The lock is deleted.\n");
		lockForLock->Release();
		return -1;
	}else if (kernelLock[lockId]->addrSpace != currentThread->space) {
		//check if the lock id is belonged to this process
		printf("The lock is not belonged to this process.\n");
		lockForLock->Release();
		return -1;
	}
	//if all the conditions are satisfied, acquire the lock
	kernelLock[lockId]->lock->Acquire();
	kernelLock[lockId]->isToBeDeleted = false;
	lockForLock->Release();
	return 0;
}

void Release_Syscall(int lockId){
	lockForLock->Acquire();
	if (lockId < 0 || lockId > MAX_LOCK) {
		//check if the lock id is within the range
		printf("Lock index is out of range.\n");
		lockForLock->Release();
		return -1;
	}else if (kernelLock[lockId] == NULL || kernelLock[lockId]->lock == NULL || kernelLock[lockId]->isDeleted) {
		//check if the lock id is deleted
		printf("The lock is deleted.\n");
		lockForLock->Release();
		return -1;
	}else if (kernelLock[lockId]->addrSpace != currentThread->space) {
		//check if the lock id is belonged to this process
		printf("The lock is not belonged to this process.\n");
		lockForLock->Release();
		return -1;
	}
	//if all the conditions are satisfied, release the lock
	kernelLock[lockId]->isToBeDeleted = true;
	kernelLock[lockId]->lock->Release();
	lockForLock->Release();
	return 0;
}

int detect_Lock(int lockId, int conditionId)
{
	if(lockId < 0 || lockId >= MAX_LOCK ||kernelLock[lockId] == NULL 
	   || kernelLock[lockId]->lock == NULL){  //The lock doesn't exist
        fprintf(stdout, "The lock %d doesn't exist or has been deleted.\n", lockId);
		return -1;
	}
	if(kernelLock[lockId]->addrSpace != currentThread->space){ //the lock doesn't belong to current thread
		fprintf(stdout, "lock %d doesn't belong to the current process.\n", lockId);
		return -1;
	}
	if(!kernelLock[lockId]->lock->isHeldByCurrentThread()){ //the current thread doesn't hold the lock
		fprintf(stdout, "current thread doesn't hold the lock %d.\n", lockId);
		return -1;
	}
	return 0;
}

int detect_CV(int lockId, int conditionId){
	if(conditionId < 0 || conditionId >= MAX_CV ||kernelCV[conditionId] == NULL
	   || kernelCV[conditionId]->cv == NULL){ //the condition variable doesn't exist
		fprintf(stdout, "Condition %d doesn't exist or has been deleted.\n", conditionId);
		return -1;
	}
	if(kernelCV[conditionId]->addrSpace != currentThread->space){ //the condition variable doesn't belong to current address
		fprintf(stdout, "Condition %d doesn't belong to the current process.\n", conditionId);
		return -1;
	}
	return 0;
}

//implement the sysCall of condition->wait
void Wait_Syscall(int lockId, int conditionId){
	lockForLock->Acquire();
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForLock->Release();
		return;
	}
	lockForCV->Acquire();
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForCV->Release();
		return;
	}
	lockForCV->Release();
    kernelCV[conditionId]->cv->Wait(kernelLock[lockId]->lock);
}

//implement the system call for signal
void Signal_Syscall(int lockId, int conditionId){
	lockForLock->Acquire();
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForLock->Release();
		return;
	}
	lockForCV->Acquire();
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForCV->Release();
		return;
	}
	kernelCV[conditionId]->cv->Signal(kernelLock[lockId]->lock); //call function:Signal
	lockForCV->Release();
	
}

//implement the system call of broadCast
void Broadcast_Syscall(int lockId, int conditionId){
	lockForLock->Acquire();
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForLock->Release();
		return;
	}
	lockForCV->Acquire();
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForCV->Release();
		return;
	}
	kernelCV[conditionId]->cv->Broadcast(kernelLock[lockId]->lock); //call function: broadCast.
	lockForCV->Release();
}

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
	switch (type) {
	    default:
		DEBUG('a', "Unknown syscall - shutting down.\n");
	    case SC_Halt:
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
		break;
	    case SC_Create:
		DEBUG('a', "Create syscall.\n");
		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Open:
		DEBUG('a', "Open syscall.\n");
		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Write:
		DEBUG('a', "Write syscall.\n");
		Write_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Read:
		DEBUG('a', "Read syscall.\n");
		rv = Read_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Close:
		DEBUG('a', "Close syscall.\n");
		Close_Syscall(machine->ReadRegister(4));
		break;
		case SC_CreateLock:
			DEBUG('a', "Create Lock syscall.\n");
			rv = CreateLock_Syscall(machine->ReadRegister(4),
									machine->ReadRegister(5));
			break;
		case SC_DestroyLock:
			DEBUG('a', "Delete Lock syscall.\n");
			rv = DeleteLock_Syscall(machine->ReadRegister(4));
			break;
		case SC_CreateCondition:
			DEBUG('a', "Create CV syscall.\n");
			rv = CreateCondition_Syscall(machine->ReadRegister(4),
										 machine->ReadRegister(5));
			break;
		case SC_DestroyCondition:
			DEBUG('a', "Delete CV syscall.\n");
			rv = DeleteCondition_Syscall(machine->ReadRegister(4));
			break;
		case SC_Acquire:
			DEBUG('a', "Acquire lock syscall.\n");
			rv = Acquire_Syscall(machine->ReadRegister(4));
			break;
		case SC_Release:
			DEBUG('a', "Release lock syscall.\n");
			rv = Release_Syscall(machine->ReadRegister(4));
			break;
		case SC_Wait:
			DEBUG('a', "Wait CV syscall.\n");
			Wait_Syscall(machine->ReadRegister(4),
							  machine->ReadRegister(5));
			break;
		case SC_Signal:
			DEBUG('a', "Signal CV syscall.\n");
			Signal_Syscall(machine->ReadRegister(4),
								machine->ReadRegister(5));
			break;
		case SC_Broadcast:
			DEBUG('a', "Broadcast CV syscall.\n");
			Broadcast_Syscall(machine->ReadRegister(4),
								   machine->ReadRegister(5));
			break;
			
	}

	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
    } else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}
