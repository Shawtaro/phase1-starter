#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "usloss.h"
#include "phase1.h"
#include "phase1Int.h"

static void DeviceHandler(int type, void *arg);
static void SyscallHandler(int type, void *arg);
static void IllegalInstructionHandler(int type, void *arg);

static int sentinel(void *arg);
int deviceSems[8];
void 
startup(int argc, char **argv)
{
    int pid;
    P1SemInit();

    // initialize device data structures
    // put device interrupt handlers into interrupt vector
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = SyscallHandler;

    /* create the sentinel process */
    int rc = P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6 , 0, &pid);
    assert(rc == P1_SUCCESS);
    // should not return
    assert(0);
    return;

} /* End of startup */

int 
P1_WaitDevice(int type, int unit, int *status) 
{
    int     result = P1_SUCCESS;
    // disable interrupts
    int prvInterrupt = P1DisableInterrupts();
    // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    if(abort == TRUE){
        return P1_WAIT_ABORTED;
    }
    if (type==USLOSS_CLOCK_DEV){
        if(unit<0||unit>=USLOSS_CLOCK_UNITS){
            return P1_INVALID_UNIT;
        }
    }else if(type == USLOSS_ALARM_DEV){
        if(unit<0||unit>=USLOSS_ALARM_UNITS){
            return P1_INVALID_UNIT;
        }
    }else if(type == USLOSS_DISK_DEV){
        if(unit<0||unit>=USLOSS_DISK_UNITS){
            return P1_INVALID_UNIT;
        }
    }else if(type == USLOSS_TERM_DEV){
        if(unit<0||unit>=USLOSS_TERM_UNITS){
            return P1_INVALID_UNIT;
        }
    }else{
        return P1_INVALID_TYPE;
    }
    // P device's semaphore
    
    // set *status to device's status
    
    // restore interrupts
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

int 
P1_WakeupDevice(int type, int unit, int status, int abort) 
{
    int     result = P1_SUCCESS;

    // disable interrupts
    int prvInterrupt = P1DisableInterrupts();
    // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // save device's status to be used by P1_WaitDevice
    
    // save abort to be used by P1_WaitDevice
    
    // V device's semaphore 
    
    // restore interrupts
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

static void
DeviceHandler(int type, void *arg) 
{
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // if clock device
    //      P1_WakeupDevice every 5 ticks
    //      P1Dispatch(TRUE) every 4 ticks
    // else
    //      P1_WakeupDevice
}

static int
sentinel (void *notused)
{
    int     pid;
    int     rc;

    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    /* start the P2_Startup process */
    rc = P1_Fork("P2_Startup", P2_Startup, NULL, 4 * USLOSS_MIN_STACK, 2 , 0, &pid);
    assert(rc == P1_SUCCESS);

    // enable interrupts
    P1EnableInterrupts();
    // while sentinel has children
    //      get children that have quit via P1GetChildStatus (either tag)
    //      wait for an interrupt via USLOSS_WaitInt
    P1_ProcInfo *info = malloc(sizeof(P1_ProcInfo));
    rc = P1_GetProcInfo(pid, info);
    int cpid;
    int status;
    //USLOSS_Console("%d\n",info->numChildren);
    while(info->numChildren!=0){
        rc = P1GetChildStatus(0,&cpid,&status);
        //rc = P1_GetProcInfo(pid, info);
        USLOSS_WaitInt();
    }
    free(info);
    USLOSS_Console("Sentinel quitting.\n");
    return 0;
} /* End of sentinel */

int 
P1_Join(int tag, int *pid, int *status) 
{
    int result = P1_SUCCESS;
    // kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // disable interrupts
    int rc = P1DisableInterrupts();

    if(tag!=0&&tag!=1){
        return P1_INVALID_TAG;
    }

    int currentPid = P1_GetPid();
    P1_ProcInfo *info = malloc(sizeof(P1_ProcInfo));
    rc = P1_GetProcInfo(currentPid, info);
    if (info->numChildren == 0){
        return P1_NO_CHILDREN;
    }
    // do
    //     use P1GetChildStatus to get a child that has quit  
    //     if no children have quit
    //        set state to P1_STATE_JOINING vi P1SetState
    //        P1Dispatch(FALSE)
    // until either a child quit or there are no more children
    do{
        rc = P1GetChildStatus(tag,pid,status);
        if(rc == P1_NO_QUIT){
            assert(P1SetState(currentPid,P1_STATE_JOINING,-1)==P1_SUCCESS);
            P1Dispatch(FALSE);
        }
    }while(rc!=P1_SUCCESS&&rc!=P1_NO_CHILDREN);
    free(info);
    return result;
}

static void
SyscallHandler(int type, void *arg) 
{
    USLOSS_Console("System call %d not implemented.\n", (int) arg);
    USLOSS_IllegalInstruction();
}

void finish(int argc, char **argv) {}
