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
    int             sid;                // semaphore on which process is blocked, if any
    int             parent;             // parent PID
    int             tag;                // process's tag
    int             children[P1_MAXPROC];// childen PIDs
    int             numChildren;        // # of children
} PCB;

static PCB processTable[P1_MAXPROC];   // the process table
static int readyQueue[P1_MAXPROC];
void P1ProcInit(void)
{
    P1ContextInit();
    // initialize everything including the processTable
    for(int i=0;i<P1_MAXPROC;i++){
        processTable[i].cid=-1;
        processTable[i].cpuTime=-1;
        processTable[i].name[0]='\0';
        processTable[i].priority=-1;
        processTable[i].state=P1_STATE_FREE;
        for (int j = 0; j < P1_MAXPROC; ++j){
            processTable[i].children[j]=-1;
        }
        processTable[i].numChildren=0;
        processTable[i].parent=-1;
        processTable[i].sid=-1;
        readyQueue[i]=-1;
    }
}

int P1_GetPid(void) 
{
    for (int pid = 0; pid < P1_MAXPROC; pid++){
        if (processTable[pid].state==P1_STATE_RUNNING){
            return pid;
        }
    }
    return -1;
}

int P1_Fork(char *name, int (*func)(void*), void *arg, int stacksize, int priority, int tag, int *pid ) 
{
    int result = P1_SUCCESS;
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // disable interrupts
    int prvInterrupt=P1DisableInterrupts();
    // check all parameters
    if (tag!=1&&tag!=0){
        return P1_INVALID_TAG;
    }

    if (priority>6||priority<1){
        return P1_INVALID_PRIORITY;
    }
    if (stacksize < USLOSS_MIN_STACK){
        return P1_INVALID_STACK;
    }
    if (name==NULL){
        return P1_NAME_IS_NULL;
    }
    if(strlen(name)>P1_MAXNAME){
        return P1_NAME_TOO_LONG;
    }
    for (int i = 0; i < P1_MAXPROC; ++i){
        if(processTable[i].state!=P1_STATE_FREE&&strcmp(processTable[i].name,name)==0){
            return P1_DUPLICATE_NAME;
        }
    }
    // check too many process
    for (; *pid < P1_MAXPROC; (*pid)++){
        if (processTable[*pid].cid==-1){
            USLOSS_Console("%d\n",*pid);
            break;
        }
    }
    if (*pid >= P1_MAXPROC){
        return P1_TOO_MANY_PROCESSES;
    }
    // create a context using P1ContextCreate
    int cid;
    P1ContextCreate((void*) func,arg,stacksize,&cid);
    // allocate and initialize PCB
    processTable[*pid].cid=cid;
    strcpy(processTable[*pid].name,name);
    processTable[*pid].priority=priority;
    processTable[*pid].tag=tag;
    processTable[*pid].state=P1_STATE_READY;
    
    int index;
    for (index = 0; readyQueue[index]!=-1; ++index){
        if(processTable[*pid].priority<processTable[readyQueue[index]].priority){
            break;
        }
    }
    for (int i = P1_MAXPROC-1; i>index; i--){
        readyQueue[i]=readyQueue[i-1];
    }
    readyQueue[index]=*pid;

    // if this is the first process or this process's priority is higher than the 
    //    currently running process call P1Dispatch(FALSE)
    int rpid=P1_GetPid();
    if (rpid==-1){
        P1Dispatch(FALSE);
    }else if(processTable[*pid].priority<processTable[rpid].priority){
        P1Dispatch(FALSE);
    }
    // re-enable interrupts if they were previously enabled
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

void 
P1_Quit(int status) 
{
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // disable interrupts
    P1DisableInterrupts();
    // remove from ready queue, set status to P1_STATE_QUIT
    
    // if first process verify it doesn't have children, otherwise give children to first process
    // add ourself to list of our parent's children that have quit
    
    // if parent is in state P1_STATE_JOINING set its state to P1_STATE_READY


    P1Dispatch(FALSE);
    // should never get here
    assert(0);
}


int 
P1GetChildStatus(int tag, int *cpid, int *status) 
{
    int result = P1_SUCCESS;
    // do stuff here
    

    return result;
}

int
P1SetState(int pid, P1_State state, int sid) 
{
    int result = P1_SUCCESS;
    // do stuff here
    if (pid<0||pid>P1_MAXPROC||processTable[pid].cid==-1){
       return P1_INVALID_PID;
    }
    if (state!=P1_STATE_READY||state!=P1_STATE_JOINING||state!=P1_STATE_BLOCKED||state!=P1_STATE_QUIT){
        return P1_INVALID_STATE;
    }
    if (state == P1_STATE_JOINING){
        // a child has quit
        for (int i = 0; i < P1_MAXPROC; ++i){
            int cpid=processTable[pid].children[i];
            if(cpid!=-1&&processTable[cpid].state==P1_STATE_QUIT){
                return P1_CHILD_QUIT;
            }
        }
    }
    processTable[pid].state=state;
    return result;
}

void
P1Dispatch(int rotate)
{
    // select the highest-priority runnable process
    // call P1ContextSwitch to switch to that process
    if (readyQueue[0]==-1){
        USLOSS_Console("No runnable processes, halting.\n");
        USLOSS_Halt(0);
    }
    int rpid=P1_GetPid();
    int nextPid=readyQueue[0];
    if (rpid==-1){
        processTable[nextPid].state=P1_STATE_RUNNING;
        P1ContextSwitch(processTable[nextPid].cid);
        for (int i = 0; i<P1_MAXPROC-1; i++){
            readyQueue[i]=readyQueue[i+1];
        }
        readyQueue[P1_MAXPROC-1]=-1;
    }else if (processTable[rpid].priority>processTable[nextPid].priority||rotate==TRUE){
        processTable[nextPid].state=P1_STATE_RUNNING;
        P1ContextSwitch(processTable[nextPid].cid);
        processTable[rpid].state=P1_STATE_READY;
        int i;
        for (i = 0; i<P1_MAXPROC; i++){
            readyQueue[i]=readyQueue[i+1];
            if (processTable[rpid].priority>=processTable[readyQueue[i]].priority){
                break;
            }
        }
        readyQueue[i]=rpid;
    }
}

int
P1_GetProcInfo(int pid, P1_ProcInfo *info)
{
    int         result = P1_SUCCESS;
    // fill in info here
    if (pid<0||pid>P1_MAXPROC||processTable[pid].cid==-1){
       return P1_INVALID_PID;
    }
    strcpy(info->name,processTable[pid].name);
    info->state=processTable[pid].state;
    info->sid=processTable[pid].sid;
    info->priority=processTable[pid].priority;
    info->tag=processTable[pid].tag;
    info->cpu=processTable[pid].cpuTime;
    info->parent=processTable[pid].parent;
    for (int i = 0; i < P1_MAXPROC; ++i){
        info->children[i]=processTable[pid].children[i];
    }
    info->numChildren=processTable[pid].numChildren;
    return result;
}

