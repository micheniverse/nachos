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
void sysCallCheckpoint();


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
            break;
          case SC_Checkpoint:
            sysCallCheckpoint();
            break;
          default:
            printf("Undefined SYSCALL %d\n", type);
            machine->WriteRegister(2, -1);
            sysCallExit();
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
        machine->WriteRegister(2, -1);
        sysCallExit();
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

  int open_spot = -1;
  int i;
  for(i = 0 ; i < MaxOpenFiles; i++){
    if(files[i] == NULL){
      if ((currentThread->inStatus == 0 || currentThread->inStatus == 2) && i == 0){}
      else if ((currentThread->outStatus == 0 || currentThread->outStatus == 2) && i == 1){}
      else{
        files[i] = current;
        open_spot = i;
        if (i == 0) {
          currentThread->inStatus = 2;
        }
        if (i == 1) {
          currentThread->outStatus = 2;
        }
        break;
      }
    }
  }

  machine->WriteRegister(2,open_spot);
  printf("%s %d\n","File descriptor ", open_spot);
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
  //processMonitor->lock();
 // processMonitor->wakeParent(threadID);
  currentThread->space->ClearMem();
  if(processMonitor->setExitStatus(threadID,result) != -1){
    DEBUG('e', "Exit threadID waking parent of %d\n", threadID);
    processMonitor->wakeParent(threadID);
    DEBUG('e', "Exit Done %d\n", threadID);
  }
  else{
    DEBUG('e',"Thread does not exist\n");
  }
  //forkExecLock->Release();


  currentThread->Finish();
  incrementPC();
  //processMonitor->unlock();
}

void sysCallJoin(){
  DEBUG('e', "Joining, initiated by user program.\n");
  int result = machine->ReadRegister(4);
  DEBUG('e', "Thread %d is Joining threadID %d\n",currentThread->getThreadId(), result);
  int exitStatus; 
  //processMonitor->lock();
  if(processMonitor->containsThread(result)){
    processMonitor->lockThreadBlock(result);
    processMonitor->sleepParent(result);
    exitStatus = processMonitor->getExitStatus(result); 
    processMonitor->unlockThreadBlock(result);
    DEBUG('e', "Parent woken %d\n", result);
  }
  //processMonitor->unlock();
  if(exitStatus == -1){
    DEBUG('e', "exitStatus is -1\n");
  }
  else{
    DEBUG('e',"%s %d \n","Exit status is: ",exitStatus);
   // processMonitor->lock();
    processMonitor->cleanUpDeadThreads(result);
    processMonitor->removeThread(result);
    //processMonitor->unlock();
  }
  machine->WriteRegister(2,exitStatus);

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

  //ASSERT(currentThread->space != NULL);
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
      else{
        file->totalLive++;
      }
    }

    machine->WriteRegister(2, fd);

    incrementPC();

    

    delete [] stringarg;               // No memory leaks.
    
}

void sysCallRead(){
  DEBUG('e', "Read, initiated by user program.\n");
  rwLock->readLock();
  //writeRead->P();
  int bufStart = machine->ReadRegister(4);
  int size = machine->ReadRegister(5);
  OpenFileId id = machine->ReadRegister(6);
  char *stringarg;
  stringarg = new(std::nothrow) char[size];  
  int result = -1;

  if (id == 1) {
    DEBUG('e', "%s\n", "Can't read from stdout");
  }
  else if (id == 0 && currentThread->inStatus == 0) {
      result = synchcon->Read(stringarg, size);
      if (result == 0) {
        DEBUG('e', "%s\n", "read failed");
        result = -1;
      }
      for(int i = 0; i<size; i++) {
        machine->mainMemory[currentThread->space->AddrTranslation(bufStart+i)] = stringarg[i];
      }

  }
  else {
    OpenFile* file = currentThread->GetFile(id);
    if (file != NULL) {
      result = file->Read(stringarg, size);
      for(int i = 0; i<size; i++) {
          machine->mainMemory[currentThread->space->AddrTranslation(bufStart+i)] = stringarg[i];
      }
    }
    
  }

  if (result == -1) {
    DEBUG('e', "%s\n", "Error reading");
  }

  machine->WriteRegister(2, result);

  incrementPC();
  delete [] stringarg; 
 // writeRead->V();
  rwLock->readUnlock();
}

void sysCallWrite(){
  DEBUG('e', "Write, initiated by user program.\n");

  rwLock->writeLock();

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

  //fprintf(stderr, "%s %s\n", "this is what I get from memory", stringarg);

  if (id == 0) {
    DEBUG('e', "%s\n", "Can't write to stdin");
  }
  else if (id == 1 && currentThread->outStatus == 0) {
    writeRead->P();
    writingReadingLock->Acquire();

    if(currentThread)
    //fprintf(stderr, "%s %s\n","  writing permission acquired to print out ",stringarg);
    for (int j = 0; j<size; j++) {
      synchcon->Write(stringarg[j], 1);
    }
    //fprintf(stderr, "%s\n","  writing permission released");
    writeRead->V();
    writingReadingLock->Release();
   // synchcon->WriteDone();
  }
  else {
    writeRead->P();
    OpenFile* file = currentThread->GetFile(id);
    //printf("%s\n","attempting to write to the file" );
    if (file != NULL)
      file->Write(stringarg, size);
    writeRead->V();
  }


  incrementPC();
 // writingReadingLock->Release();
  rwLock->writeUnlock();
  //fprintf(stderr, "%s\n","write Lock released");
}

void sysCallClose(){
  DEBUG('e', "Close, initiated by user program.\n");
  int fd;

  fd = machine->ReadRegister(4);

  OpenFile* result = currentThread->RemoveFile(fd);
  if(result == NULL) {
    DEBUG('e', "%s\n", "error closing a file");
    if (fd == 0) {
      currentThread->inStatus = 1;
    }
    if (fd == 1) {
      currentThread->outStatus = 1;
    }
  }
  else {
    if (fd == 0) {
      currentThread->inStatus = 1;
    }
    if (fd == 1) {
      currentThread->outStatus = 1;
    }
    result->totalLive--;
    if(result->totalLive == 0)
      delete result;    
    DEBUG('e', "Deleting file descriptor %d with only %d of it left open\n", fd,result->totalLive);
  }
  incrementPC();
}

void runMachine(int spaceId){
  DEBUG('e', "Child, initiated by user program. with id %d\n",spaceId);
  currentThread->RestoreUserState();
  currentThread->space->RestoreState();  
  rootSema->V();
  machine->Run();
}

void sysCallFork(){

  //forkExec->P();
  DEBUG('e', "%s\n", "Checking Lock if Acquired from Fork\n");
  forkExecLock->forkLock();
  DEBUG('e', "%s\n", "Lock Acquired from Fork\n");
  DEBUG('e', "Fork, initiated by user program.\n");
  Thread *forkedThread = new Thread("Forked Thread");
  int spaceId;
  //To do copy the parents address space and open files.
  for (int i = 0; i < MaxOpenFiles; i++){
    OpenFile *temp = currentThread->openFiles[i];
    if(forkedThread->openFiles[i] != NULL){
      temp->totalLive++;
      forkedThread->openFiles[i] = temp;
    }
  }
  forkedThread->inStatus = currentThread->inStatus;
  forkedThread->outStatus = currentThread->outStatus;
  forkedThread->space = new(std::nothrow) AddrSpace(currentThread->space);

  if (forkedThread->space->errorState != true) {
    DEBUG('e',"successful creation of addrspace");
    spaceId = processMonitor->addThread(forkedThread, currentThread);
  }
  else {
    spaceId = -1;
    DEBUG('e',"CREATION FAILURE");
  }


  if(spaceId ==-1){
    DEBUG('e',"CREATION FAILURE");
    incrementPC();
  }
  else {
    incrementPC();
    currentThread->SaveUserState();

    machine->WriteRegister(2,0);

    forkedThread->SaveUserState();
    forkedThread->Fork((VoidFunctionPtr) runMachine,spaceId);

    rootSema->P();
  }


  machine->WriteRegister(2,spaceId);

  forkExecLock->forkUnlock();
  DEBUG('e',"Fork Thread Unlocks \n");
}

//Extra info needed for the system!
// Number of children
// The parent-process id
// The parent is waiting to join?
//Resources 
 
void sysCallExec(){
  //forkExec->P();
  DEBUG('e', "Execute, initiated by user program.\n");

  DEBUG('e', "Attempt to grab lock\n");

  forkExecLock->forkLock();
  DEBUG('e', "Lock Acquired from Exec\n");
  char *fileName;
  int argStart = machine->ReadRegister(4);
  int argvStart = machine->ReadRegister(5);
  fileName = new(std::nothrow) char[128];
  int i;
  char** argv;

  OpenFile *exec;
  int curAddr;
  int argc;


  if (currentThread->space != NULL) {
    
    for (i=0; i<127; i++) {
      if ((fileName[i]=machine->mainMemory[currentThread->space->AddrTranslation(argStart)]) == '\0') break;
      argStart++;
    }

    DEBUG('e', "The pointers to the filename have been read in\n");


    argv = new(std::nothrow) char*[128];
    argc=0;
    
    for ( i = 0; i < 127; i++) {
      argv[i] = new(std::nothrow) char[128];
      curAddr = currentThread->space->ReadMemory(argvStart, 4);

      if (curAddr == 0 || curAddr == -1) {
        break;
      }
      for (int j=0; j<127; j++) {
        if ((argv[i][j]=machine->mainMemory[currentThread->space->AddrTranslation(curAddr)]) == '\0') {
          break;
        }
        curAddr++;
      }
      argvStart+=4;
      argc++;
     // fprintf(stderr, "%s %s\n","here is what we get from reading in", argv[i] );
    }
  }
  else {
    curAddr = -1;
  }

  if (curAddr != -1){

    exec = fileSystem->Open(fileName);
  }
  else {
    exec = NULL;
  }

  //Invoke it through machine running.
  incrementPC();

  if(exec != NULL){

    int argvAddr[argc+1];
    int status;

    //fprintf(stderr, "%s\n", "Trying to ExecFunction");
    status = currentThread->space->ExecFunc(exec);

    if (status == 2) {
      currentThread->openFiles[0] = exec;
      currentThread->inStatus = 2;

      currentThread->space->InitRegisters();   // set the initial register values
      currentThread->space->RestoreState();    // load page table register

    }
    else if (status == 3) {
      // do the checkpoint things
    }
    else if (status != -1) {
      //fprintf(stderr, "%s\n", "Done to ExecFunction");
      delete exec;    //delete the executable

      currentThread->space->InitRegisters();   // set the initial register values
      currentThread->space->RestoreState();    // load page table register

      DEBUG('e', "Registers have been inited and restored\n");
      int sp = machine->ReadRegister(StackReg);

      int len = strlen(fileName) + 1;

      sp -= len;

      for (i = 0; i < len; i++) {
        machine->mainMemory[currentThread->space->AddrTranslation(sp+i)] = fileName[i];
      }
      argvAddr[0] = sp;

      DEBUG('e', "filename loaded\n");

      for (i=0; i<argc; i++) {
          len = strlen(argv[i]) + 1;
          sp -= len;
          for (int j = 0; j < len; j++){
            machine->mainMemory[currentThread->space->AddrTranslation(sp+j)] = argv[i][j];
          }
         
          argvAddr[i+1] = sp;
          


        DEBUG('e', "Reading the data into the memory %s\n", argv[i]);
      }

      sp = sp & ~3;
      
      argc++;
      sp -= sizeof(int) *4;

      for(i = 0; i<argc; i++) {
        *(unsigned int *)&machine->mainMemory[currentThread->space->AddrTranslation((sp+i*4))] = (unsigned int) argvAddr[i];
      }

      DEBUG('e',"About to Write\n");
      machine->WriteRegister(4, argc);

      machine->WriteRegister(5, sp);

      machine->WriteRegister(StackReg, sp-8);
    }
    else {
      DEBUG('e', "%s\n", "Error from ExecFunc");
      machine->WriteRegister(2,-1);
    }
    forkExecLock->forkUnlock();
   DEBUG('e', "%s\n", "Exec Lock Released\n");


  }
  else{
    DEBUG('e', "%s\n", "Error Opening File");
    machine->WriteRegister(2,-1);
    forkExecLock->forkUnlock();
  }
}

void sysCallCheckpoint() {
  int whence;
  char *stringarg;
  stringarg = new(std::nothrow) char[128];
  whence = machine->ReadRegister(4);
  int returnVal;
  returnVal = 0;

  for (int i=0; i<127; i++) {
    if ((stringarg[i]=machine->mainMemory[currentThread->space->AddrTranslation(whence)]) == '\0') break;
    whence++;
  }
  stringarg[127]='\0'; 

  OpenFile *file = fileSystem->Open(stringarg);

  if(file == NULL) {
    DEBUG('e', "%s\n", "no file found, about to create");
    bool result = fileSystem->Create(stringarg,1);

    if(result == false) {
      DEBUG('e',"issue creating the file");
    }

    file = fileSystem->Open(stringarg);
    if (file == NULL) {
      returnVal = -1;
    }
  }
  if (returnVal != -1) {
    // need a cookie to demonstrate that the file is a checkpoint file
    // cookie is 0xbaebee
    char buffer[10];
    file->Write( "CHECKDIS\n", sizeof("CHECKDIS\n"));
    printf("%x\n", "CHECKDIS");
    incrementPC();
    currentThread->SaveUserState();
    int numPages = currentThread->space->getNumPages();
    int* registers = currentThread->getUserRegisters();
    TranslationEntry* pageTable = currentThread->space->getPageTable();
    machine->WriteRegister(2, 1);
    for (int i = 0; i < NumTotalRegs; i++) {
      file->Write((char*)registers[i], sizeof((char*)registers[i]));
      printf("%d\n",registers[i] );
      file->Write("|", sizeof("|"));
    }
    file->Write("\n", sizeof("\n"));
    file->Write((char *)numPages, sizeof((char *)numPages));
    file->Write("\n", sizeof("\n"));
    for (int i = 0; i < numPages; i++) {
      for (int j = 0; j < 128; j++) {
        //write in pages

      }
      // file->Write((char*)pageTable[i].virtualPage, sizeof((char*)pageTable[i].virtualPage));
      // file->Write(" ", sizeof(" "));
      // file->Write((char*)pageTable[i].physicalPage, sizeof((char*)pageTable[i].physicalPage));
      // file->Write(" ", sizeof(" "));
      // file->Write((char*)pageTable[i].valid, sizeof((char*)pageTable[i].valid));
      // file->Write(" ", sizeof(" "));
      // file->Write((char*)pageTable[i].readOnly, sizeof((char*)pageTable[i].readOnly));
      // file->Write(" ", sizeof(" "));
      // file->Write((char*)pageTable[i].use, sizeof((char*)pageTable[i].use));
      // file->Write(" ", sizeof(" "));
      // file->Write((char*)pageTable[i].dirty, sizeof((char*)pageTable[i].dirty));
      // file->Write("|", sizeof("|"));
    }

    file->Write("\n", sizeof("\n"));

    file->Read(buffer, sizeof((char *)0x00baebee));
    printf("%d\n", (int)buffer );

    machine->WriteRegister(2, 0);
  }
  else {
    DEBUG('e', "%s\n", "Error opening/creating a file");
    machine->WriteRegister(2, -1);
  }
}
#endif