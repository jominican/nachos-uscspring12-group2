// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"
#include "bitmap.h"
#include "machine.h"

//BitMap phyMemBM = new BitMap(NumPhysPages);
BitMap* phyMemBM =  new BitMap(NumPhysPages);
Lock* phyMemBMLock = new Lock("phyMemBMLock");

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    unsigned int i, size;
	unsigned int stackSize; //the num of pages of a stack for one thread
	acquire_num = 0;
	lockIdArray = new int[10];
    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
	stackSize = divRoundUp(UserStackSize,PageSize);
    numPages = divRoundUp(size, PageSize) + stackSize;
                                                // we need to increase the size
						// to leave room for the stack
    size = numPages * PageSize;
	
    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
	// first, set up the translation 
    pageTable = new TranslationEntry[numPages + stackSize*Max_Threads];
	stackArrays = new int[Max_Threads];	// the total pages for a user program.
	virToPhy = new int[numPages + stackSize*Max_Threads];
	
	// The thread id for the first thread, only the first thread of the "so called process" will call this constructor
	int currentThreadID = 0;
    for (i = 0; i < numPages; i++) {
		// Try to find a unused memory page in the physical memory.
		phyMemBMLock->Acquire();
		int phyMemPage = phyMemBM->Find();
		phyMemBMLock->Release();
		DEBUG('a', "The index of the physical memory is %d.\n", phyMemPage);
		
		if(-1 == phyMemPage){
			printf("No enough physical memory to allocate.\n");
			return;
		}else{
			virToPhy[i] = phyMemPage;
		}
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = phyMemPage;	// support for multi-programming.
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
						// a separate page, we could set its 
						// pages to be read-only
		//if(i >= (numPages - stackSize)){
		//	stackArrays[i] = currentThreadID;
		//}else{
		//	stackArrays[i] = -1;
		//}
		executable->ReadAt(&(machine->mainMemory[phyMemPage*PageSize]), PageSize, i*PageSize+noffH.code.inFileAddr);
    }
	stackArrays[currentThreadID] = numPages - stackSize; //the beginning stack page for the first page
	//printf("start index of thread %d is %d", currentThreadID, numPages - stackSize);
    
	/*
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
	*/
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::AllocateStackPages
// allocate stack for a newly created thread
//
// We  copy the whole pageTable of the process to the pageTable of this thread,
// then we add 8 pages as the stack at the end of the pageTable of the thread
//----------------------------------------------------------------------

int
AddrSpace::AllocateStackPages(int threadID)
{
	if(threadID < 0){
		printf("Error in the 'threadID' when allocating stack for the thread.\n");
		return -1;
	}
	int stackSize = divRoundUp(UserStackSize,PageSize); 
	
	int stackEndIndex = numPages + stackSize; //the end index of the stack for this thread
	for(int i = numPages; i < stackEndIndex; ++i){
		phyMemBMLock->Acquire();
		int phyMemPage = phyMemBM->Find(); //Find an available physical memory page.
		phyMemBMLock->Release();
		DEBUG('a', "The index of the physical memory is %d.\n", phyMemPage);
		if(phyMemPage == -1){
			printf("No enough physical memory to allocate for a stack.\n");
			return -1;
		}
		else{
			virToPhy[i] = phyMemPage;
		}
		//printf("PageTable [%d].\n", i);
		pageTable[i].virtualPage = i;
		pageTable[i].physicalPage = phyMemPage;	//Allocate the physical mem page to one page of the stack.
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
						// a separate page, we could set its 
						// pages to be read-only
              
	}
	//printf("start index of thread %d is %d", threadID, numPages);
	stackArrays[threadID] = numPages; //To indicate that the beginning virtual memory page stores the stack of the thead whose id is "threadID"
	numPages = stackEndIndex;//update the numPages.
	//printf("The stackEndIndex is: %d.\n", numPages);
	return (numPages*PageSize-16);
}

//------------------------------------------------------------------------------
//AddrSpace::deleteStackPages
//delete the stack for the thread
//
//-------------------------------------------------------------------------------
void
AddrSpace::deleteStackPages(int thread_id){
	int r = stackArrays[thread_id]; //the first page of the stack for a thread
	int stackSize = divRoundUp(UserStackSize,PageSize);
	phyMemBMLock->Acquire();
	//printf("Before deleting stack pages for process %d; thread %d start Page %d.\n", process_id, thread_id,r);
	for(int i = r; i != r+stackSize; ++i)
	{
		//printf("%d.\n",i);
		int phyMemPage = pageTable[i].physicalPage; //get the physical memory page
		//printf("%d +++++.\n",i);
		phyMemBM->Clear(phyMemPage); //clear physical memory page.
	}
	//printf("After deleting stack pages of thread %d.\n", thread_id);
	phyMemBMLock->Release();
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
