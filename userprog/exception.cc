// exception.cc 
//  Entry point into the Nachos kernel from user programs.
//  There are two kinds of things that can cause control to
//  transfer back to here from user code:
//
//  syscall -- The user code explicitly requests to call a procedure
//  in the Nachos kernel.  Right now, the only function we support is
//  "Halt".
//
//  exceptions -- The user code does something that the CPU can't handle.
//  For instance, accessing memory that doesn't exist, arithmetic errors,
//  etc.  
//
//  Interrupts (which can also cause control to transfer from user
//  code into the Nachos kernel) are handled elsewhere.
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
#include <string.h>

void sysCallExec();
void sysCallHalt();
void sysCallExit();
void sysCallFork();
void sysCallJoin();
void sysCallCreate();
void sysCallOpen();
void sysCallClose();
void sysCallRead();
void sysCallWrite();
void sysCallDup();
void incrementPC();



#ifdef USE_TLB

//----------------------------------------------------------------------
// HandleTLBFault
//      Called on TLB fault. Note that this is not necessarily a page
//      fault. Referenced page may be in memory.
//
//      If free slot in TLB, fill in translation info for page referenced.
//
//      Otherwise, select TLB slot at random and overwrite with translation
//      info for page referenced.
//
//----------------------------------------------------------------------


void
HandleTLBFault(int vaddr)
{
  int vpn = vaddr / PageSize;
  int victim = Random() % TLBSize;
  int i;

  stats->numTLBFaults++;

  // First, see if free TLB slot
  for (i=0; i<TLBSize; i++)
    if (machine->tlb[i].valid == false) {
      victim = i;
      break;
    }

  // Otherwise clobber random slot in TLB

  machine->tlb[victim].virtualPage = vpn;
  machine->tlb[victim].physicalPage = vpn; // Explicitly assumes 1-1 mapping
  machine->tlb[victim].valid = true;
  machine->tlb[victim].dirty = false;
  machine->tlb[victim].use = false;
  machine->tlb[victim].readOnly = false;
}

#endif

//----------------------------------------------------------------------
// ExceptionHandler
//  Entry point into the Nachos kernel.  Called when a user program
//  is executing, and either does a syscall, or generates an addressing
//  or arithmetic exception.
//
//  For system calls, the following is the calling convention:
//
//  system call code -- r2
//    arg1 -- r4
//    arg2 -- r5
//    arg3 -- r6
//    arg4 -- r7
//
//  The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//  "which" is the kind of exception.  The list of possible exceptions 
//  are in machine.h.
//----------------------------------------------------------------------

#ifdef CHANGED
void
ExceptionHandler(ExceptionType which)
{

    int type = machine->ReadRegister(2);

    //DEBUG('e', "%s %d %d\n", "Here is the type", type, which);

    int totalThreads;
    //fprintf(stderr, "%s %d\n", "Here is the type", type);

    switch (which) {
      case SyscallException:
        switch (type) {
          case SC_Halt:
            sysCallHalt();
            break;
          case SC_Exit:
            sysCallExit();
            break;
          case SC_Exec:
            sysCallExec();
            break;
          case SC_Join:
            sysCallJoin();
            break;
          case SC_Create:
            sysCallCreate();
            break;
          case SC_Open:
            sysCallOpen();
            break;
          case SC_Read:
            sysCallRead();
            break;
          case SC_Write:
            sysCallWrite();
            break;
          case SC_Close:
            sysCallClose();
            break;
          case SC_Fork:
            sysCallFork();
            break;
          case SC_Dup:
            sysCallDup();
          default:
            printf("Undefined SYSCALL %d\n", type);
            ASSERT(false);
            break;
        }
      break;
      #ifdef USE_TLB
            case PageFaultException:
        HandleTLBFault(machine->ReadRegister(BadVAddrReg));
        break;
      #endif

      default: 
        printf("Unexpected exception caught for user mode %d %d\n",which,type);
        interrupt->Halt();
        break;
    }
}

void incrementPC()
{
  int tmp;
  tmp = machine->ReadRegister(PCReg);
  machine->WriteRegister(PrevPCReg, tmp);
  tmp = machine->ReadRegister(NextPCReg);
  machine->WriteRegister(PCReg, tmp);
  tmp += 4;
  machine->WriteRegister(NextPCReg, tmp);
}

void sysCallDup(){
  DEBUG('e', "Dup, initiated by user program.\n");
  OpenFileId fileId = machine->ReadRegister(4);
  OpenFile **files = currentThread->openFiles;
  OpenFile *current = files[fileId];

  int open_spot;
  int i;
  for(i = 2 ; i < MaxOpenFiles; i++){
    if(files[i] == NULL){
      files[i] = current;
      open_spot = i;
      break;
    }
  }
  if(i >= MaxOpenFiles){
    DEBUG('e', "Could not duplicate file\n");
    machine->WriteRegister(2,-1);
  }
  else{
    machine->WriteRegister(2,open_spot);
  }
  incrementPC();
}

void sysCallHalt(){
  DEBUG('e', "Shutdown, initiated by user program.\n");
  interrupt->Halt();
}

void sysCallExit(){
  DEBUG('e', "Exit, initiated by user program.\n");
  int threadID = currentThread->getThreadId();
  DEBUG('e', "Exit threadID %d\n", threadID);
 // ASSERT(threadID != -1);
  int result = machine->ReadRegister(4);
  processMonitor->lock();
  if(processMonitor->setExitStatus(threadID,result) != -1){
    DEBUG('e', "Exit threadID waking parent of %d\n", threadID);
    processMonitor->wakeParent(threadID);
  }
  else{
    DEBUG('e',"Thread does not exist");
  }
  DEBUG('e', "Exit Done %d\n", threadID);
  processMonitor->unlock();
  currentThread->Finish();
}

void sysCallJoin(){
  DEBUG('e', "Joining, initiated by user program.\n");
  int result = machine->ReadRegister(4);
  DEBUG('e', "Thread %d is Joining threadID %d\n",currentThread->getThreadId(), result);
  int exitStatus; 
  processMonitor->lock();
  if(processMonitor->containsThread(result)){
    processMonitor->lockThreadBlock(result);
    processMonitor->wakeParent(result);
    exitStatus = processMonitor->getExitStatus(result); 
    processMonitor->unlockThreadBlock(result);
    DEBUG('e', "Parent woken %d\n", result);
  }
  processMonitor->unlock();
  if(exitStatus == -1){
    DEBUG('e', "exitStatus is -1");
  }
  else{
    DEBUG('e',"%s %d \n","Exit status is: ",exitStatus);
    processMonitor->lock();
    processMonitor->cleanUpDeadThreads(result);
    processMonitor->removeThread(result);
    processMonitor->unlock();
    machine->WriteRegister(2,exitStatus);
  }

  incrementPC();
}

void sysCallCreate(){
  DEBUG('e', "Create, initiated by user program.\n");
   // Grab kernel memory sufficient to hold argument string. Note that
   // this imposes a restriction on the length of the string.
  int whence;
  char *stringarg;
  stringarg = new(std::nothrow) char[128];                                         

  whence = machine->ReadRegister(4); // whence is VIRTUAL address
                                     //   of first byte of arg string.
                                     //   IN THIS CASE, virtual=physical.

// Copy the string from user-land to kernel-land.

  ASSERT(currentThread->space != NULL);
  for (int i=0; i<127; i++) {
    if ((stringarg[i]=machine->mainMemory[currentThread->space->AddrTranslation(whence)]) == '\0') break;
    whence++;
  }
  stringarg[127]='\0';              // Effectively truncates a string
                                     //   if it's too long. Better,
                                     //   get string length and error
                                       //   before copy if too long.


    bool result = fileSystem->Create(stringarg,1);

    if(result == false) {
      DEBUG('e',"issue creating the file");
    }
    delete [] stringarg;               // No memory leaks.
    
    incrementPC();

}

void sysCallOpen(){
  DEBUG('e', "Open, initiated by user program.\n");
  int whence;
  char *stringarg;
  stringarg = new(std::nothrow) char[128];
  int fd;                                       

    whence = machine->ReadRegister(4); // whence is VIRTUAL address
                                       //   of first byte of arg string.
                                       //   IN THIS CASE, virtual=physical.


  // Copy the string from user-land to kernel-land.

  ASSERT(currentThread->space != NULL);
  for (int i=0; i<127; i++) {
    if ((stringarg[i]=machine->mainMemory[currentThread->space->AddrTranslation(whence)]) == '\0') break;
    whence++;
  }
  stringarg[127]='\0';              // Effectively truncates a string
                                       //   if it's too long. Better,
                                       //   get string length and error
                                       //   before copy if too long.


    OpenFile *file = fileSystem->Open(stringarg);

    if(file == NULL) {
      DEBUG('e', "%s\n", "no file found");
      fd = -1;
    }
    else {
      fd = currentThread->AddFile(file);
      if (fd == -1){
        DEBUG('e', "%s\n", "Failed to add file to openfile array");
      }
    }

    machine->WriteRegister(2, fd);

    incrementPC();

    

    delete [] stringarg;               // No memory leaks.
    
}

void sysCallRead(){
  //DEBUG('e', "Read, initiated by user program.\n");
  int bufStart = machine->ReadRegister(4);
  int size = machine->ReadRegister(5);
  OpenFileId id = machine->ReadRegister(6);
  char *stringarg;
  stringarg = new(std::nothrow) char[size];  
  int result = -1;

  if (id == 1) {
    DEBUG('e', "%s\n", "Can't read from stdout");
  }
  else if (id == 0) {
      result = synchcon->Read(stringarg, size);
      if (result == 0) {
        DEBUG('e', "%s\n", "read failed");
        result = -1;
      }
      for(int i = 0; i<size; i++) {
        machine->mainMemory[bufStart+i] = stringarg[i];
      }

  }
  else {
    OpenFile* file = currentThread->GetFile(id);
    if (file != NULL) {
      result = file->Read(stringarg, size);
      for(int i = 0; i<size; i++) {
          machine->mainMemory[bufStart+i] = stringarg[i];
      }
    }
    
  }

  if (result == -1) {
    DEBUG('e', "%s\n", "Error reading");
  }

  machine->WriteRegister(2, result);

  incrementPC();
  delete [] stringarg; 
}

void sysCallWrite(){
 // DEBUG('e', "Write, initiated by user program.\n");
  int bufStart = machine->ReadRegister(4);
  int size = machine->ReadRegister(5);
  OpenFileId id = machine->ReadRegister(6);
  char *stringarg;
  stringarg = new(std::nothrow) char[128];  

  ASSERT(currentThread->space != NULL);
  for (int i=0; i<size; i++) {
    if ((stringarg[i]=machine->mainMemory[currentThread->space->AddrTranslation(bufStart)]) == '\0') break;
    bufStart++;
  }
  stringarg[127]='\0'; 

 // fprintf(stderr, "%s %s\n", "this is what I get from memory", stringarg);


  if (id == 0) {
    DEBUG('e', "%s\n", "Can't write to stdin");
  }
  else if (id == 1) {
    for (int j = 0; j<size; j++) {
      synchcon->Write(stringarg[j], 1);
    }
  }
  else {
    OpenFile* file = currentThread->GetFile(id);
    file->Write(stringarg, size);
  }


  incrementPC();
}

void sysCallClose(){
  DEBUG('e', "Close, initiated by user program.\n");
  int fd;

  fd = machine->ReadRegister(4);

  OpenFile* result = currentThread->RemoveFile(fd);
  if(result == NULL) {
    DEBUG('e', "%s\n", "error closing a file");
  }
  else {
    delete result;    
  }

  incrementPC();
}
void runMachine(){
  DEBUG('e', "Child, initiated by user program.\n");
  currentThread->RestoreUserState();
  currentThread->space->RestoreState();  
  machine->Run();
}

void sysCallFork(){

  forkExec->Acquire();
  DEBUG('e', "Fork, initiated by user program.\n");
  Thread *forkedThread = new Thread("Forked Thread");
  //To do copy the parents address space and open files.
  for (int i = 0; i < MaxOpenFiles; i++)
    forkedThread->openFiles[i] = currentThread->openFiles[i];
  forkedThread->space = new(std::nothrow) AddrSpace(currentThread->space);
  //Todo: What to do with the space id
  //int arg = machine->ReadRegister(4);
  processMonitor->lock();
  int spaceId = processMonitor->addThread(forkedThread, currentThread);
  processMonitor->unlock();

  if(spaceId ==-1){
    DEBUG('e',"CREATION FAILURE");
    return;
  }
  incrementPC();
  currentThread->SaveUserState();

  machine->WriteRegister(2,0);

  forkedThread->SaveUserState();
  forkExec->Release();
  forkedThread->Fork((VoidFunctionPtr) runMachine,0);
  printf("Thread %d SpaceID in exception %d\n", currentThread->getThreadId(),spaceId);
  processMonitor->sleepParent(spaceId);
  printf("SpaceID in exception after waking %d\n", spaceId);
  //Return to parent process

  machine->WriteRegister(2,spaceId);
}

//Extra info needed for the system!
// Number of children
// The parent-process id
// The parent is waiting to join?
//Resources 

void sysCallExec(){
  forkExec->Acquire();
  DEBUG('e', "Execute, initiated by user program.\n");
  char *fileName;
  int argStart = machine->ReadRegister(4);
  int argvStart = machine->ReadRegister(5);
  fileName = new(std::nothrow) char[128];
  int i;
  char** argv;

 // fprintf(stderr, "%s %d\n", "this is the initial arg", argvStart);

  //fprintf(stderr, "%s %d\n", "doing some stuff", argc);

  // if (argc < 0) {
  //   DEBUG('a', "Argc is negative, switching it to 0\n");
  //   argc = 0;
  // }


  ASSERT(currentThread->space != NULL);
  for (i=0; i<127; i++) {
    if ((fileName[i]=machine->mainMemory[currentThread->space->AddrTranslation(argStart)]) == '\0') break;
    argStart++;
  }


  // char *start = new(std::nothrow) char[128];
  // for (i = 0; i < 127; i++){
  //   if ((start[i] = machine->mainMemory[currentThread->space->AddrTranslation(argvStart)]) == '\0') break;
  //   //fprintf(stderr, "%s %c\n","attempting to read", start[i] );
  // }

  int curAddr;
  //fprintf(stderr, "%s %s\n","here is the start", start );

  argv = new(std::nothrow) char*[128];
  int argc=0;
  //fprintf(stderr, "%s %c\n", "Do i read in any data from register 5?", machine->mainMemory[currentThread->space->AddrTranslation(argvData)]);
  for ( i = 0; i < 127; i++) {
    argv[i] = new(std::nothrow) char[128];
    curAddr = currentThread->space->ReadMemory(argvStart, 4);
  //  fprintf(stderr, "%s %d\n","here is the curaddr", curAddr );
    if (curAddr == 0) {
      break;
    }
    for (int j=0; j<127; j++) {
      //fprintf(stderr, "%s %c\n", "let's try before the loop", machine->mainMemory[currentThread->space->AddrTranslation(curAddr)] );
      if ((argv[i][j]=machine->mainMemory[currentThread->space->AddrTranslation(curAddr)]) == '\0') {
    //    fprintf(stderr, "%s\n", "breaking out");
        break;
      }
      curAddr++;
    }
    argvStart+=4;
    argc++;
   // fprintf(stderr, "%s %s\n","here is what we get from reading in", argv[i] );
  }

 // fprintf(stderr, "%s %d\n", "here is the num of args", argc);
  //Initialize its registers
  //fprintf(stderr, "%s %s\n", "about to do the open", fileName);
  OpenFile *exec = fileSystem->Open(fileName);

  //Invoke it through machine running.
  incrementPC();

  if(exec != NULL){

    int argvAddr[argc+1];

    //fprintf(stderr, "%s\n", "Trying to ExecFunction");
    currentThread->space->ExecFunc(exec);
    //fprintf(stderr, "%s\n", "Done to ExecFunction");
    delete exec;    //delete the executable

    currentThread->space->InitRegisters();   // set the initial register values
    currentThread->space->RestoreState();    // load page table register

    int sp = machine->ReadRegister(StackReg);

    int len = strlen(fileName) + 1;

    sp -= len;

    for (i = 0; i < len; i++) {
      machine->mainMemory[currentThread->space->AddrTranslation(sp+i)] = fileName[i];
    }
    argvAddr[0] = sp;


   // fprintf(stderr, "%s %s\n","the argv[0] val", argv[0] );
   // fprintf(stderr, "And now the filename %s\n", fileName);
    int initialVal = 0;
    if (!strcmp(argv[0], fileName)) {
      initialVal = 1;
     // fprintf(stderr, "%s\n", "we've found a filename match!");
    }
    for (i=initialVal; i<argc; i++) {
      if (argv[i] != fileName) {
        len = strlen(argv[i]) + 1;
        sp -= len;
        for (int j = 0; j < len; j++){
          machine->mainMemory[currentThread->space->AddrTranslation(sp+j)] = argv[i][j];
          //fprintf(stderr, "We've read this char into memory %c\n",argv[i][j] );
        }
        if (initialVal == 0)
          argvAddr[i+1] = sp;
        else
          argvAddr[i] = sp;
        
        // Jose looks to do this once?
      }

    //  fprintf(stderr, "Reading the data into the memory %s\n", argv[i]);
    }

    sp = sp & ~3;
    
    if (initialVal == 0) {
      argc++;
    }

    sp -= sizeof(int) *4;

    for(i = 0; i<argc; i++) {
      *(unsigned int *)&machine->mainMemory[currentThread->space->AddrTranslation((sp+i*4))] = (unsigned int) argvAddr[i];
    }
    DEBUG('a',"About to Write\n");
    machine->WriteRegister(4, argc);
   // printf("%d\n",argc );
    //fprintf(stderr, "%s\n", "did we write one");
    machine->WriteRegister(5, sp);
   // fprintf(stderr, "%s\n", "what about this one");
    machine->WriteRegister(StackReg, sp-8);
   // fprintf(stderr, "%s\n", "last one");

    // machine->Run();


    // //Create a new Thread
    // Thread *newProcess = new Thread("Executed Program Thread");
    // //Allocate a new address space object for the new Thread.
    // space = new(std::nothrow) AddrSpace(exec);
    // newProcess->space = space;

    // //return the address space identifier to the calling process
    // machine->WriteRegister(2,newProcess->getThreadId());

    // //saveUser state just in case of failure (Maybe we dont need this?)
    // currentThread->SaveUserState();

    // //Prep registers to look like just starting
    // currentThread->space->InitRegisters();
    // currentThread->space->RestoreState();
    // //Run the Process
    // newProcess->Fork(machine->Run(),0);
    // //Inheriting open files from former execution????

    // //Should not have reached here so return failure.
    // currentThread->RestoreUserState();
    // DEBUG('e', "%s\n", "Forking has failed somehow");
    // machine->WriteRegister(2,-1);



  }
  else{
    DEBUG('e', "%s\n", "Error Opening File");
    //fprintf(stderr, "%s\n", "oh my god");
    machine->WriteRegister(2,-1);
  }
  forkExec->Release();
}
#endif