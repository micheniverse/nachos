// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "openfile.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "bitmap.h"
#include <new>
#include "processMonitor.h"


// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock
extern ProcessMonitor *processMonitor;
extern unsigned int gspaceID;
extern Semaphore *forkExec;
extern Semaphore *writeRead;
extern Semaphore *rootSema;
extern Lock *writingReadingLock;
extern ForkExecLock *forkExecLock;
extern Condition *wrCondition;
extern ReadWriteLock *rwLock;
extern  int consoleIn;
extern  int consoleOut;
extern  int newconsoleIn;
extern  int newconsoleOut;


#ifdef USER_PROGRAM
#ifdef CHANGED
#include "machine.h"
#include "synchconsole.h"
extern Machine* machine;	// user program memory and registers
extern SynchConsole* synchcon;
extern BitMap *pagemap;

#endif
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
