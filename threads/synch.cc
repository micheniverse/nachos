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

Semaphore::Semaphore(const char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new(std::nothrow) List;
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

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
#ifdef CHANGED
Lock::Lock(const char* debugName) 
{
	//binarysem = new Semaphore(debugName, 1);
	name = debugName;
	locker = NULL;
	currentState = FREE;
	lockqueue = new(std::nothrow) List;
}
Lock::~Lock() 
{
	delete lockqueue;
}
void Lock::Acquire() 
{
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	if(currentState == BUSY){
		lockqueue->Append((void *)currentThread);
		currentThread->Sleep();
	}
	else{
		currentState = BUSY;
		locker = currentThread;
	}

	(void) interrupt->SetLevel(oldLevel);
}
void Lock::Release() 
{
	Thread *thread;
	
	ASSERT(isHeldByCurrentThread());

	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	thread = (Thread *) lockqueue->Remove();
	if(thread != NULL){
		scheduler->ReadyToRun(thread);
	}
	currentState = FREE;
	locker = NULL;

	(void) interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread() 
{
	if(locker == currentThread) {
		return true;
	}
	return false;
}

Condition::Condition(const char* debugName) 
{ 
	//threadqueue = new List<Semaphore *>;
	threadqueue = new(std::nothrow) List;
	name = debugName;
}
Condition::~Condition() 
{ 
	delete threadqueue;
}

void Condition::Wait(Lock* conditionLock) 
{ 
	//ASSERT(false);
	ASSERT(conditionLock->isHeldByCurrentThread());

	//Semaphore *temp = new Semaphore(name,0);
	threadqueue->Append((Thread *) currentThread);
	
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	
	//printf("Lock realsed\n");
	conditionLock->Release();				// release the lock before going to sleep to avoid deadlock
	currentThread->Sleep();

	(void) interrupt->SetLevel(oldLevel);
	
	//printf("Lock re-acquired %s\n",conditionLock->name);								// tell the thread to sleep
		conditionLock->Acquire();				// re-acquire the lock upon wakeup (given to us in specs)
	//delete temp;
}
void Condition::Signal(Lock* conditionLock) 
{
	ASSERT(conditionLock->isHeldByCurrentThread());

	//Semaphore *temp; 
	Thread *temp;
	if(!threadqueue->IsEmpty())
	{
		temp = (Thread *) threadqueue->Remove();
		scheduler->ReadyToRun(temp);
	}

}
void Condition::Broadcast(Lock* conditionLock) 
{ 
	ASSERT(conditionLock->isHeldByCurrentThread());

	Thread *temp;
	while(!threadqueue->IsEmpty()){
		temp = (Thread *) threadqueue->Remove();
		scheduler->ReadyToRun(temp);
	}

}
#endif //CHANGED