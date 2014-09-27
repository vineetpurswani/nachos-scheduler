// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

	void
StartProcess(char *filename)
{
	OpenFile *executable = fileSystem->Open(filename);
	AddrSpace *space;

	if (executable == NULL) {
		printf("Unable to open file %s\n", filename);
		return;
	}
	space = new AddrSpace(executable);    
	currentThread->space = space;

	delete executable;			// close file

	space->InitRegisters();		// set the initial register values
	space->RestoreState();		// load page table register

	machine->Run();			// jump to the user progam
	ASSERT(FALSE);			// machine->Run never returns;
	// the address space exits
	// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

	void 
ConsoleTest (char *in, char *out)
{
	char ch;

	console = new Console(in, out, ReadAvail, WriteDone, 0);
	readAvail = new Semaphore("read avail", 0);
	writeDone = new Semaphore("write done", 0);

	for (;;) {
		readAvail->P();		// wait for character to arrive
		ch = console->GetChar();
		console->PutChar(ch);	// echo it!
		writeDone->P() ;        // wait for write to finish
		if (ch == 'q') return;  // if q, quit
	}
}

	void
BatchStartFunction(int dummy)
{
	currentThread->Startup();
	DEBUG('t', "Running thread \"%d\" for the first time\n", currentThread->GetPID());
	// Call the start_time function over her
	machine->Run();
}

int countlines(char *filename)
{
	// count the number of lines in the file called filename                                    
	FILE *fp = fopen(filename,"r");
	int ch=0;
	int lines=0;

	if (fp == NULL)
		return 0;

	lines++;
	while ((ch = fgetc(fp)) != EOF)
	{
		if (ch == '\n')
			lines++;
	}
	fclose(fp);
	return lines;
}

void
RunBatch(char *filename){
	int lines = countlines(filename);
	FILE * executable = fopen(filename, "r");
	ASSERT(executable != NULL);
	int i,flag=0;
	char processName[100];
	int processPriority;
	OpenFile *programfile;
	Thread *thread;

	//printf("Number of Lines: %d\n",lines);
	lines--;
	//printf("Parent Thread: %d\n",currentThread->GetPID());

	for(i=0;i<lines;i++){
		fscanf(executable,"%s",processName);
		if(fgetc(executable) == '\n'){
			processPriority = 100;
			flag=1;
		}
		else{
			fscanf(executable,"%d",&processPriority);
		}

		programfile = fileSystem->Open(processName);
		ASSERT(programfile != NULL);
		thread = new Thread(processName, processPriority, false);
		//thread = new Thread(processName);
		thread->space = new AddrSpace(programfile);
		thread->space->InitRegisters();
		thread->SaveUserState();
		thread->StackAllocate(BatchStartFunction, 0);
		thread->Schedule();
		delete programfile;	
		printf("Child Thread[%d]: %d whose parent is: %d\n",i,thread->GetPID(),thread->GetPPID());
	}

	fclose(executable);
	printf("Parent Exiting...\n");
	exitThreadArray[0] = true;
	currentThread->Exit(false,0);
}

