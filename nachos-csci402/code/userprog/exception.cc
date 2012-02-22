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
#include "console.h"
#include "addrspace.h"
#include "bitmap.h"
#include "synch.h"

#include <stdio.h>
#include <iostream>

// Define the maximum number of the user program.
#define MAX_PROCESS 3
#define MAX_LOCK 5001
#define MAX_CV  5001

using namespace std;

// System call Lock.
struct KernelLock{
	Lock* lock;
	AddrSpace* addrSpace;
	bool isDeleted;	// the current state of the kernel lock.
	bool isToBeDeleted;	// valid to delete.
};

// System call CV.
struct KernelCV{
	Condition* cv;
	AddrSpace* addrSpace;
	bool isDeleted;	// the current state of the kernel cv.
	bool isToBeDeleted;	// valid to delete.
};

// Address struct to run a thread.
struct Addr {
	AddrSpace* space;
	int vaddr; 
	int nvaddr; 	// 8 stack pages address.
};

// User program.
struct Process{
	int processID;
	int totalThread;	// for thread ID.
	int activeThread;	// for check the live thread number.
};

int processID = 0;
KernelLock* kernelLock[MAX_LOCK];
KernelCV* kernelCV[MAX_CV];
Process* process = new Process[MAX_PROCESS];

Lock* lockForLock = new Lock("lockForLock");
Lock* lockForCV = new Lock("lockForCV");
Lock* lockForProcess = new Lock("lockForProcess");	
//Lock* lockForThread = new Lock("lockForThread");
//Lock* lockForExit = new Lock("lockForExit");
BitMap* lockBM = new BitMap(MAX_LOCK);
BitMap* cvBM = new BitMap(MAX_CV);
//BitMap lockBM(MAX_LOCK);
//BitMap cvBM(MAX_CV);

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

// The implementation of the CreateLock() system call.
int CreateLock_Syscall(unsigned int vaddr, int len)
{
	// Kernel buffer to load the name of the Lock.
	char* buf = new char[len+1];
	
	// Check for the allocation of the kernel buffer.
	if(!buf){
		printf("%s", "Can't allocate kernel buffer in CreateLock.\n");
		return -1;
	}
	
	buf[len] = '\0';
	// Check for the copy in operation from vaddr to buf with len bytes..
	if(-1 == copyin(vaddr, len, buf)){
		printf("%s","Bad pointer passed to CreateLock.\n");
		delete[] buf;
		return -1;
	}
	
	lockForLock->Acquire();
	
	int index = lockBM->Find();
	if(-1 == index){	// whether there is a available Kernel Lock.
		printf("%s", "Can't find available Kernel Lock.\n");
		lockForLock->Release();
		return -1;
	}
	
	DEBUG('t', "Create Lock \"%s\"\n", buf);
	
	// Construct a new Kernel Lock with the name of buf, located in position 
	// index of the kernelLock table.
	Lock* lock = new Lock(buf);
	kernelLock[index] = new KernelLock;
	kernelLock[index]->lock = lock;
	kernelLock[index]->addrSpace = currentThread->space;
	kernelLock[index]->isDeleted = false;
	kernelLock[index]->isToBeDeleted = true;
	lockForLock->Release();
	
	// Return the position (the index) of the kernel Lock.
	delete[] buf;
	return index;
}

// The implementation of the DestroyLock() system call.
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
	
	DEBUG('t', "Delete Lock %d\n", lockIndex);
	
	// Check the status of the kernel lock.
	if(kernelLock[lockIndex]->isToBeDeleted){
		// Here, we can run the delete operation.
		delete kernelLock[lockIndex]->lock;
		lockBM->Clear(lockIndex);	// clear the BitMap bit.
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

// The implementation of the CreateCondition() system call.
int CreateCondition_Syscall(unsigned int vaddr, int len)
{
	// Kernel buffer to load the name of the CV.
	char* buf = new char[len+1];
	
	// Check for the allocation of the kernel buffer.	
	if(!buf){
		printf("%s", "Can't allocate kernel buffer in CreateCondition.\n");
		return -1;
	}
	
	buf[len] = '\0';
	// Check for the copy in operation from vaddr to buf with len bytes..
	if(-1 == copyin(vaddr, len, buf)){
		printf("%s","Bad pointer passed to CreateCondition.\n");
		delete[] buf;
		return -1;
	}
	
	lockForCV->Acquire();
	int index = cvBM->Find();
	if(-1 == index){ // whether there is a available Kernel CV.
		printf("%s", "Can't find available Kernel Condition Variable.\n");
		lockForCV->Release();
		return -1;
	}
	
	DEBUG('t', "Create Condition \"%s\"\n", buf);
	
	// Construct a new Kernel CV with the name of buf, located in position 
	// index of the kernelCV table.
	Condition* cv = new Condition(buf);
	kernelCV[index] = new KernelCV;
	kernelCV[index]->cv = cv;
	kernelCV[index]->addrSpace = currentThread->space;
	kernelCV[index]->isDeleted = false;
	kernelCV[index]->isToBeDeleted = true;
	lockForCV->Release();
	
	// Return the position (the index) of the kernel CV.
	delete[] buf;
	return index;
}

// The implementation of the DestroyCondition() system call.
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
	
	DEBUG('t', "Delete Condition %d\n", cvIndex);
	
	// Check the status of the kernel cv.
	if(kernelCV[cvIndex]->isToBeDeleted){
		// Here, we can run the delete operation.
		delete kernelCV[cvIndex]->cv;
		cvBM->Clear(cvIndex);	// clear the BitMap bit.
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

// The implementation of the Acquire() system call.
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
	
	DEBUG('t', "Acquire Lock %d\n", lockId);
	
	//if all the conditions are satisfied, acquire the lock
	kernelLock[lockId]->lock->Acquire();
	kernelLock[lockId]->isToBeDeleted = false;	// acquired by a user program, can't be deleted.
	lockForLock->Release();
	return 0;
}

// The implementation of the Release() system call.
int Release_Syscall(int lockId){
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
	
	DEBUG('t', "Release Lock %d\n", lockId);
	
	//if all the conditions are satisfied, release the lock
	kernelLock[lockId]->isToBeDeleted = true;
	kernelLock[lockId]->lock->Release();	// released by a user program, can be deleted.
	lockForLock->Release();
	return 0;
}

int detect_Lock(int lockId, int conditionId)
{
	if(lockId < 0 || lockId >= MAX_LOCK || kernelLock[lockId] == NULL 
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
	if(conditionId < 0 || conditionId >= MAX_CV || kernelCV[conditionId] == NULL
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

//The implementation of the Wait() system call.
int Wait_Syscall(int lockId, int conditionId){
	lockForLock->Acquire();
	
	DEBUG('t', "Check if Lock %d is validated\n", lockId);
	
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForLock->Release();
		return -1;
	}
	
	lockForCV->Acquire();
	
	DEBUG('t', "Check if Condition %d is validated\n", conditionId);
	
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForCV->Release();
		return -2;
	}
	lockForCV->Release();
	
	DEBUG('t', "Wait on Condition %d with Lock %d\n", conditionId, lockId);
	
	lockForProcess->Acquire();
	--process[currentThread->processID].activeThread;
	lockForProcess->Release();
    kernelCV[conditionId]->cv->Wait(kernelLock[lockId]->lock);
	return 0;
}

// The implementation of the Signal() system call.
int Signal_Syscall(int lockId, int conditionId){
	lockForLock->Acquire();
	
	DEBUG('t', "Check if Lock %d is validate\n", lockId);
	
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForLock->Release();
		return -1;
	}
	lockForCV->Acquire();
	
	DEBUG('t', "Check if Condition %d is validate\n", conditionId);
	
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForCV->Release();
		return -2;
	}
	
	DEBUG('t', "Signal Condition %d with Lock %d\n", conditionId, lockId);
	lockForCV->Release();
	int r = kernelCV[conditionId]->cv->Signal(kernelLock[lockId]->lock); //call function:Signal
	lockForProcess->Acquire();
	process[currentThread->processID].activeThread += r;
	lockForProcess->Release();
	
	return 0;
	
}

// The implementation of the BroadCast() system call.
int BroadCast_Syscall(int lockId, int conditionId){
	lockForLock->Acquire();
	
	DEBUG('t', "Check if Lock %d is validate\n", lockId);
	
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForLock->Release();
		return -1;
	}
	lockForCV->Acquire();
	
	DEBUG('t', "Check if Condition %d is validate\n", conditionId);
	
	if(detect_Lock(lockId, conditionId) == -1){ //detect data validation
		lockForCV->Release();
		return -2;
	}
	lockForCV->Release();
	DEBUG('t', "BroadCast Condition %d with Lock %d\n", conditionId, lockId);
	int r = 0;
	r += kernelCV[conditionId]->cv->Broadcast(kernelLock[lockId]->lock); //call function: broadCast.
	lockForProcess->Acquire();
	process[currentThread->processID].activeThread += r;
	lockForProcess->Release();
	return 0;
}

void Run_KernelProcess(int space)
{
	DEBUG('t', "Run kernel process.\n");
	((AddrSpace*)space)->InitRegisters();
	((AddrSpace*)space)->RestoreState();
	//currentThread->space->InitRegisters();
	//currentThread->space->RestoreState();
	machine->Run();	// run the program stored in the "physical" memory.
}

// The implementation of the Exec() system call.
// In fact, we just return 0 when Exec() succeed, and -1 when fail.
typedef int SpaceId;
//SpaceId 
void Exec_Syscall(unsigned int vaddr, int len)
{
	// Kernel buffer to load the name of the executable file.
	char* buf = new char[len+1];
	
	// Check for the allocation of the kernel buffer.	
	if(!buf){
		printf("%s", "Can't allocate kernel buffer in Exec.\n");
		//return -1;
		return;
	}
	
	buf[len] = '\0';
	// Check for the copy in operation from vaddr to buf with len bytes..
	if(-1 == copyin(vaddr, len, buf)){
		printf("%s","Bad pointer passed to Exec.\n");
		delete[] buf;
		//return -1;
		return;
	}
	
	// Open the executable file (user program).
	OpenFile* executable = fileSystem->Open(buf);
	if(NULL == executable){
		printf("Unable to open file %s.\n", buf);
		delete[] buf;
		//return -1;
		return;
	}
	
	// Load the executable file into physical memory.
	lockForProcess->Acquire();
	AddrSpace* space = new AddrSpace(executable);	// need to check return value?
	
	// Initialize the process information.
	if(processID < MAX_PROCESS){
		process[processID].processID = processID;	// ugly naming.
		process[processID].totalThread = 1;	// the number of kernel threads in the user program.
		process[processID].activeThread = 1;	// the number of active kernel threads in the user program.
	}else{ 	// exceed the maximum number of user programs we support.
		printf("Can't create more than %d user programs.\n", MAX_PROCESS);
		delete[] buf;
		delete executable;
		lockForProcess->Release();
		//return -1;
		return;
	}
	
	// Fork a new kernel thread to take the place of the user program.
	Thread* thread = new Thread("User program");
	thread->space = space;
	thread->threadID = 0;	// first thread for the user program.
	thread->processID = process[processID++].processID;
	DEBUG('t', "The information of the user program is space: %d, threadID: %d, processID: %d.\n", 
			thread->space, thread->threadID, thread->processID);
	delete[] buf;
	delete executable;
	lockForProcess->Release();
	
	// Last statement in a function.
	thread->Fork(Run_KernelProcess, (int)(thread->space));
	// Why cannot use the following method?
	//thread->space->InitRegisters();		// set the initial register values
    //thread->space->RestoreState();		// load page table register
	//machine->Run();			// jump to the user progam
	// Execute user program successed.
	//return 0;
	return;
}

void Run_KernelThread(int addr)
{
	// Init registers.
	for(int i = 0; i< NumTotalRegs; ++i){
		machine->WriteRegister(i, 0);
	}
	machine->WriteRegister(PCReg, ((Addr*)addr)->vaddr);
	machine->WriteRegister(NextPCReg, ((Addr*)addr)->vaddr + 4);
	machine->WriteRegister(StackReg, ((Addr*)addr)->nvaddr);
	((Addr*)addr)->space->RestoreState();
	
	machine->Run();
}

// The implementation of the Fork() system call.
//int 
void Fork_Syscall(unsigned int vaddr)
{
	//lockForThread->Acquire();
	lockForProcess->Acquire();
	
	// Check the validation of the virtual address.
	if(0 == vaddr || vaddr < 0 || (vaddr%4) != 0){
		printf("Invalid virtual address.\n");
		//lockForThread->Release();
		lockForProcess->Release();
		//return -1;
		return;
	}
	
	Thread* thread = new Thread("Thread");
	thread->space = currentThread->space;
	thread->threadID = process[currentThread->processID].totalThread;
	thread->processID = currentThread->processID;
	++process[thread->processID].totalThread;	// add one thread to the current user program.
	++process[thread->processID].activeThread;	// add one active thread to the current user program.
	// Allocate another 8 pages for the thread.
	int nvaddr = thread->space->AllocateStackPages(thread->threadID);
	DEBUG('t', "The information of the forked thread is space: %d, stack space: %d, threadID: %d, processID: %d.\n", 
			thread->space, nvaddr, thread->threadID, thread->processID);
	
	// Address struct to pass to the Run_ routine.
	Addr* addr = new Addr;
	addr->space = thread->space;
	addr->vaddr = vaddr;
	addr->nvaddr = nvaddr;
	//lockForThread->Release();
	lockForProcess->Release();
	
	thread->Fork(Run_KernelThread, (int)addr);	
	
	//return 0;
	return;
}

int activeThreadNum()
{
	int sum = 0;
	for(int i = 0; i < processID; ++i){
		sum += process[i].activeThread;
	}
	return sum;
}


// The implementation of the Exit() system call.
void Exit_Syscall(int status)
{
	// Exit with signal.
	if(status){
		printf("User program exit with signal %d.\n", status);
		return;
	}
	//lockForExit->Acquire();
	lockForProcess->Acquire();
	//--process[currentThread->processID]->totalThread;
	--process[currentThread->processID].activeThread;
	
	DEBUG('t', "Call Exit().\n");
	
	if(process[currentThread->processID].activeThread != 0){		// not the last thread for neither of the situations.
		// Need to deallocated the 8 pages stack?
    	currentThread->space->deleteStackPages(currentThread->threadID);//delete the memory stackPages of the current thread.	
		lockForProcess->Release();
		currentThread->Finish();
		return;
	}else{		// the last thread for the current user program.
		// When end a user program, we should deallocated all of the 
		// resources it holds, such as the Locks and CVs.
		// The Locks and CVs may have been destroyed by the user program,
		// we just check and make sure the deallocation.
		
		// Deallocate the Locks.
		lockForLock->Acquire();
		for(int i = 0; i < MAX_LOCK; ++i){
			if(kernelLock[i] != NULL && kernelLock[i]->addrSpace == currentThread->space)
				if(lockBM->Test(i)){
					delete kernelLock[i]->lock;
					lockBM->Clear(i);	// clear the BitMap bit.
					kernelLock[i]->addrSpace = NULL;
					kernelLock[i]->isDeleted = true;
					kernelLock[i]->isToBeDeleted = false;	// can't be deleted.
				}
		}
		lockForLock->Release();
		
		// Deallocate the CVs.
		lockForCV->Acquire();
		for(int i = 0; i < MAX_CV; ++i){
			if(kernelCV[i] != NULL && kernelCV[i]->addrSpace == currentThread->space)
				if(cvBM->Test(i)){
					delete kernelCV[i]->cv;
					cvBM->Clear(i);	// clear the BitMap bit.
					kernelCV[i]->addrSpace = NULL;
					kernelCV[i]->isDeleted = true;
					kernelCV[i]->isToBeDeleted = false;	// can't be deleted.
				}
		}
		lockForCV->Release();
		
		delete currentThread->space;	// must be deleted at last.
		if(activeThreadNum() != 0){	// not the last thread for all of the user programs.
			lockForProcess->Release();
			currentThread->Finish();
			return;
		}
	}
	
	// The last thread for all of the user programs.
	if(0 == activeThreadNum()){
		lockForProcess->Release();
		interrupt->Halt();
	}
	
	lockForProcess->Release();
	// Exit succeed.
	return;
}

// The implementation of the Yield() system call.
void Yield_Syscall()
{
	currentThread->Yield();
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
			rv = Wait_Syscall(machine->ReadRegister(4),
							  machine->ReadRegister(5));
			break;
		case SC_Signal:
			DEBUG('a', "Signal CV syscall.\n");
			rv = Signal_Syscall(machine->ReadRegister(4),
								machine->ReadRegister(5));
			break;
		case SC_BroadCast:
			DEBUG('a', "BroadCast CV syscall.\n");
			rv = BroadCast_Syscall(machine->ReadRegister(4),
								   machine->ReadRegister(5));
			break;
		case SC_Exit:
			DEBUG('a', "Exit syscall.\n");
			Exit_Syscall(machine->ReadRegister(4));
			break;
		case SC_Exec:
			DEBUG('a', "Exec syscall.\n");
			Exec_Syscall(machine->ReadRegister(4),
							  machine->ReadRegister(5));
			break;
		case SC_Fork:
			DEBUG('a', "Fork syscall.\n");
			Fork_Syscall(machine->ReadRegister(4));
			break;
		case SC_Yield:
			DEBUG('a', "Yield syscall.\n");
			Yield_Syscall();
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
