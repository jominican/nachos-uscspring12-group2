// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Lock::Lock
//	Initialize a Lock instance, when a Lock object is created,
//	the Lock state is FREE (true), the holder of the Lock (hostThread)
//	is NULL.
//----------------------------------------------------------------------

Lock::Lock(char* debugName)
{
	name       = debugName;
	value      = true; 	// we can omit this field, using hostThread instead.
	hostThread = NULL;
	queue      = new List;
}

//----------------------------------------------------------------------
// Lock::~Lock
//	When deallocate a Lock object, we should manually delete the queue. 
//----------------------------------------------------------------------

Lock::~Lock()
{
	delete queue;
}

//----------------------------------------------------------------------
// Lock::Acquire
//	The current thread is trying to acquire the Lock object.
//	1: no thread can acquire the same Lock object twice.
//	2: when the Lock is FREE, get the Lock.
//	3: when the Lock is BUSY, wait for the Lock.
//----------------------------------------------------------------------

void 
Lock::Acquire()
{
	IntStatus old = interrupt->SetLevel(IntOff); 	// disable interrupt

	if(isHeldByCurrentThread()){ 	// 	no thread can acquire the same Lock object twice 
		(void)interrupt->SetLevel(old);
		return;
	}

	if(true == value){ 	// 	the Lock is FREE
						// 	the same to hostThread == NULL
		value = false;
		hostThread = currentThread;
	}else{ 	// 	the Lock is BUSY
		queue->Append((void*)currentThread); 	// 	before going to sleep, add myself to the waiting queue
		currentThread->Sleep();		
	}

	(void)interrupt->SetLevel(old); 	// 	restore interrupt
}

//----------------------------------------------------------------------
// Lock::Release
//	The current thread is trying to release the Lock object.
//	1: only the Lock holder can release the Lock object.
//	2: some threads are in the waiting queue.
//	3: no threads are in the waiting queue.	
//----------------------------------------------------------------------

void 
Lock::Release() 
{
	IntStatus old = interrupt->SetLevel(IntOff); 	// disable interrupt

	if(!isHeldByCurrentThread()){ 	// 	only the Lock holder can release the Lock object 	
		fprintf(stderr, "Error: Only the Lock holder can release the Lock!\n");
		(void)interrupt->SetLevel(old);
		return;
	}

	if(!queue->IsEmpty()){ 	// 	some threads are waiting for the Lock object
		Thread* thread = (Thread*)queue->Remove();
		scheduler->ReadyToRun(thread);
		hostThread = thread; 	// give the thread the Lock ownership
	}else{ 	// 	no threads are in the waiting queue
		value = true;
		hostThread = NULL; 	// 	clear the Lock ownership
	}

	(void)interrupt->SetLevel(old); 	// restore interrupt
}

//----------------------------------------------------------------------
// Lock::isHeldByCurrentThread
//	Check whether the current thread in the CPU is the same as the 
//	holder of the Lock object.
//----------------------------------------------------------------------

bool 
Lock::isHeldByCurrentThread()
{
	return (currentThread == hostThread);
}

//----------------------------------------------------------------------
// Condition::Condition
//	Initialize a Condition Variable instance, when a Condition Variable
//	object is created, the waiting lock is NULL. 
//----------------------------------------------------------------------

Condition::Condition(char* debugName) 
{ 
	name        = debugName;
	waitingLock = NULL;
	queue       = new List;
}

//----------------------------------------------------------------------
// Condition::~Condition
//	When deallocating a Condition Variable object, we should delete
//	the queue field manually.
//----------------------------------------------------------------------

Condition::~Condition() 
{ 
	delete queue;
}

//----------------------------------------------------------------------
// Condition::Wait
//	Wait operation in a Condition Variable object.
//	1: the conditionLock could NOT be NULL
//	2: if the waitingLock is NULL, we directly assign it with the 
//		conditionLock
//	3: a Condition Variable should be binded to the same Lock object
//	4: OK to wait
//----------------------------------------------------------------------

void
Condition::Wait(Lock* conditionLock) 
{ 
	IntStatus old = interrupt->SetLevel(IntOff); 	// disable interrupt

	if(NULL == conditionLock){ 	// 	invalid conditionLock
		fprintf(stderr, "Error: Condition Lock could NOT be NULL!\n");
		(void)interrupt->SetLevel(old);
		return;
	}

	if(NULL == waitingLock){ 	// the first waiting thread
		waitingLock = conditionLock;
	}

	if(waitingLock != conditionLock){ 	// two different Lock objects for the same C.V. object
		fprintf(stderr, "Error: A Condition Variable should be binded to the same Lock object!\n");
		(void)interrupt->SetLevel(old);
		return;
	}

	// OK to wait
	conditionLock->Release(); 	// allow other threads to get the Lock
	queue->Append((void*)currentThread); 	// add myself to the waiting queue
	currentThread->Sleep();
	conditionLock->Acquire();
	
	(void)interrupt->SetLevel(old); 	// restore interrupt
}

//----------------------------------------------------------------------
// Condition::Signal
//	Signal operation in a Condition Variable object.
//	1: no threads are in the waiting queue
//	2: a Condition Variable should be binded to the same Lock object
//	3: OK to wakeup
//	4: no more waiting threads
//----------------------------------------------------------------------

void 
Condition::Signal(Lock* conditionLock) 
{ 
	IntStatus old = interrupt->SetLevel(IntOff); 	// disable interrupt

	if(queue->IsEmpty()){ 	// no threads are in the waiting queue
		(void)interrupt->SetLevel(old);
		return;
	}

	if(waitingLock != conditionLock){ 	// invalid condition lock
		fprintf(stderr, "Error: A Condition Variable should be binded to the same Lock object!\n");
		(void)interrupt->SetLevel(old);
		return;
	}

	Thread* thread = (Thread*)queue->Remove(); 	// 	wakeup one waiting thread
	scheduler->ReadyToRun(thread);

	if(queue->IsEmpty()) waitingLock = NULL; 	// no more waiting threads
 	
	(void)interrupt->SetLevel(old); 	// restore interrupt
}

//----------------------------------------------------------------------
// Condition:: Broadcast
//	Broadcast operation in a Condition Variable object.
//----------------------------------------------------------------------

void 
Condition::Broadcast(Lock* conditionLock) 
{ 
	while(!queue->IsEmpty()) Signal(conditionLock); 	// 	invoke the Signal operation
}
