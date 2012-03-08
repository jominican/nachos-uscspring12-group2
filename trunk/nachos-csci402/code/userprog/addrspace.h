// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256
#define Max_Threads 200	// maximum number of threads in a so-called user program.

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

	int AllocateStackPages(int); // Allocate (8) stack pages for the thread.
    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch
    Table fileTable;			// Table of openfiles
	void deleteStackPages(int); //delete stacks
	int acquire_num;
	int* lockIdArray; //the array to store the id for the locks in the user program.
 private:
    TranslationEntry *pageTable;	// Assume linear page table translation
	int* stackArrays;
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
	int* virToPhy;	// Virtual memory to physical memory mapping.
};

#endif // ADDRSPACE_H
