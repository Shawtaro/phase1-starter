/*
Phase 1b
*/

#include "phase1Int.h"
#include "usloss.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


typedef struct PCB {
    int             cid;                // context's ID
    int             cpuTime;            // process's running time
    char            name[P1_MAXNAME+1]; // process's name
    int             priority;           // process's priority
    P1_State        state;              // state of the PCB
    // more fields here
	// some of these fields come from phase1.h, P1_ProcInfo struct, not sure if all are needed
	int pid;                // process's ID
	int tag;
	int sid;                    // semaphore on which process is blocked, if any
	int numChildren;
	int numQuitChildren;
	int children[P1_MAXPROC];
	int quitChildren[P1_MAXPROC];
	PCB* parent;
	PCB* next;
	PCB* prev;
	int parentPID;
} PCB;

static PCB processTable[P1_MAXPROC];   // the process table

void P1ProcInit(void)
{
	// Check Kernel Mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		printf("User mode\n");
		USLOSS_IllegalInstruction();
    } 
	// initialize contexts
    P1ContextInit();
    for (int i = 0; i < P1_MAXPROC; ++i)
    {
        processTable[i].state=P1_STATE_FREE;
	processTable[i].numChildren = 0;
	//TODO
    }

}

int P1_GetPid(void) 
{
	// Check Kernel Mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		printf("User mode\n");
		USLOSS_IllegalInstruction();
    } 
	// search for running process
	for (int i = 0; i < P1_MAXPROC; ++i)
	        if (processTable[i].state==P1_STATE_RUNNING)
			break;

    return processTable[i].pid;
}

int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int tag, int *pid ) 
{
    int result = P1_SUCCESS;

	// Check Kernel Mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		printf("User mode\n");
		USLOSS_IllegalInstruction();
    } 

    // disable interrupts
	int interruptsPreviouslyEnabled = P1DisableInterrupts();
	// TODO below
    // check all parameters
    // create a context using P1ContextCreate
    // allocate and initialize PCB
    // if this is the first process or this process's priority is higher than the 
    //    currently running process call P1Dispatch(FALSE)
	// TODO Above
    // re-enable interrupts if they were previously enabled
	if(interruptsPreviouslyEnabled )
		P1EnableInterrupts();
    return result;
}

void 
P1_Quit(int status) 
{
    // check for kernel mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		printf("User mode\n");
		USLOSS_IllegalInstruction();
    } 

    // disable interrupts
	int interruptsPreviouslyEnabled = P1DisableInterrupts();

    // remove from ready queue, set status to P1_STATE_QUIT

	// search for running process
	for (int i = 0; i < P1_MAXPROC; ++i)
	        if (processTable[i].state==P1_STATE_RUNNING)
			break;
	processTable[i].prev->next=processTable[i].next;
	processTable[i].next = NULL;
	processTable[i].state = P1_STATE_QUIT;
	
    // if first process verify it doesn't have children, otherwise give children to first process
	if (processTable[i].pid==0 && processTable[i].numChildren > 0){
		printf("First process quitting with children, halting\n");
		USLOSS_Halt(1);

	}
	for (int i = 0; i < numChildren; ++i){
	        processTable[i].parent->children[processTable[i].parent->numChildren+i]=processTable[i].children[i];
		processTable[i].parent->numChildren++;
	}
    // add ourself to list of our parent's children that have quit
	processTable[i].parent->quitChildren[numQuitChildren+1]=processTable[i].pid;
	processTable[i].parent->numQuitChildren;
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY
	if(processTable[i].parent->state==P1_STATE_JOINING)
		processTable[i].parent->state=P1_STATE_READY;


    P1Dispatch(FALSE);
    // should never get here
    assert(0);
}


int 
P1GetChildStatus(int tag, int *cpid, int *status) 
{

    return result;
}

int
P1SetState(int pid, P1_State state, int sid) 
{
    int result = P1_SUCCESS;
	for (int i = 0; i < P1_MAXPROC; ++i)
	        if (processTable[i].pid==pid)
			break;
	if(i==P1_MAXPROC)
		result = P1_INVALID_PID;
	else{
		switch(state){
			case P1_STATE_READY :
				processTable[i].state = P1_STATE_READY;
				processTable[i].sid = sid;
				break;
			case P1_STATE_JOINING :
				// TODO
				// check if a child quit, if so
				// result = P1_CHILD_QUIT

				processTable[i].state = P1_STATE_JOINING;
				processTable[i].sid = sid;

				break;
			case P1_STATE_BLOCKED :
				processTable[i].state = P1_STATE_BLOCKED ;
				processTable[i].sid = sid;
				break;
			case P1_STATE_QUIT :
				processTable[i].state = P1_STATE_QUIT;
				processTable[i].sid = sid;
				break;
			default :
				result = P1_INVALID_STATE;
		}
	}

    return result;
}

void
P1Dispatch(int rotate)
{
    // select the highest-priority runnable process
	// Check Kernel Mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		printf("User mode\n");
		USLOSS_IllegalInstruction();
    } 
	int runningPID = P1_GetPID();
	int maxPriority = 0;
	int maxIndex = 0;
	// search for highest priority process
	for (int i = 0; i < P1_MAXPROC; ++i)
	        if (processTable[i].priority>maxPriority && processTable[i].state == P1_STATE_READY){
			maxPriority = processTable[i].priority
			maxIndex = i
		}
	if(maxPriority == 0 ){// no runnable processes, bc priority is only 1,...6
		printf("No runnable processes, halting.\n");
		USLOSS_Halt(0);
	}
    // call P1ContextSwitch to switch to that process
	if (processTable[maxIndex].pid == runningPID && rotate == true){
		P1ContextSwitch(processTable[maxIndex].next->cid);
	}

}

int
P1_GetProcInfo(int pid, P1_ProcInfo *info)
{
    int         result = P1_SUCCESS;
    
	for (int i = 0; i < P1_MAXPROC; ++i)
	        if (processTable[i].pid==pid)
			break;
	if(i==P1_MAXPROC)
		result = P1_INVALID_PID;
	else{
		info -> name = processTable[i].name;
		info -> sid = processTable[i].sid;
		info -> priority = processTable[i].priority;
		info -> tag = processTable[i].tag;
		info -> cpu = processTable[i].cputTime;
		info -> parent = processTable[i].parentPID;
		info -> children = processTable[i].children;
		info -> numChildren = processTable[i].numChildren;
	}

    return result;
}







