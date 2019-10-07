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
typedef struct{
    int sem;
    int status;
    int abort;
}Device;

Device devices[8];
int ticks;
void 
startup(int argc, char **argv)
{
    int pid;
    int rc;
    P1SemInit();
    ticks = 0;
    // initialize device data structures
    for (int i = 0; i < 8; ++i){
        devices[i].status=-1;
        devices[i].abort=FALSE;
    }
    rc=P1_SemCreate("Alarm",0,&devices[0].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Clock",0,&devices[1].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Disk_0",0,&devices[2].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Disk_1",0,&devices[3].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Term_0",0,&devices[4].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Term_1",0,&devices[5].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Term_2",0,&devices[6].sem);
    assert(rc == P1_SUCCESS);
    rc=P1_SemCreate("Term_3",0,&devices[7].sem);
    assert(rc == P1_SUCCESS);
    // put device interrupt handlers into interrupt vector
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = SyscallHandler;
    USLOSS_IntVec[USLOSS_CLOCK_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_ALARM_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_DISK_INT] = DeviceHandler;
    USLOSS_IntVec[USLOSS_TERM_INT] = DeviceHandler;
    //USLOSS_IntVec[USLOSS_ILLEGAL_INT] = IllegalInstructionHandler;

    /* create the sentinel process */
    rc = P1_Fork("sentinel", sentinel, NULL, USLOSS_MIN_STACK, 6 , 0, &pid);
    assert(rc == P1_SUCCESS);
    // should not return
    assert(0);
    return;

} /* End of startup */

int 
P1_WaitDevice(int type, int unit, int *status) 
{
    int     result = P1_SUCCESS;
    int     rc;
    // disable interrupts
    int prvInterrupt = P1DisableInterrupts();
    // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    if(unit<0){
        return P1_INVALID_UNIT;
    }

    // P device's semaphore
    // set *status to device's status
    switch(type){
        case USLOSS_CLOCK_DEV:
            if(unit>=USLOSS_CLOCK_UNITS){
                return P1_INVALID_UNIT;
            }
            if (devices[1].abort){
                return P1_WAIT_ABORTED;
            }
            rc=P1_P(devices[1].sem);
            assert(rc == P1_SUCCESS);
            *status=devices[1].status;
            break;
        case USLOSS_ALARM_DEV:
            if(unit>=USLOSS_ALARM_UNITS){
                return P1_INVALID_UNIT;
            }
            if (devices[0].abort){
                return P1_WAIT_ABORTED;
            }
            rc=P1_P(devices[0].sem);
            assert(rc == P1_SUCCESS);
            *status=devices[0].status;
            break;
        case USLOSS_DISK_DEV:
            if(unit>=USLOSS_DISK_UNITS){
                return P1_INVALID_UNIT;
            }
            if (devices[2+unit].abort){
                return P1_WAIT_ABORTED;
            }
            rc=P1_P(devices[2+unit].sem);
            assert(rc == P1_SUCCESS);
            *status=devices[2+unit].status;
            break;
        case USLOSS_TERM_DEV:
            if(unit>=USLOSS_TERM_UNITS){
                return P1_INVALID_UNIT;
            }
            if (devices[4+unit].abort){
                return P1_WAIT_ABORTED;
            }
            rc=P1_P(devices[4+unit].sem);
            assert(rc == P1_SUCCESS);
            *status=devices[4+unit].status;
            break;
        default:
            return P1_INVALID_TYPE;
    }
    
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
    int rc;
    // disable interrupts
    int prvInterrupt = P1DisableInterrupts();
    // check kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }

    if(unit<0){
        return P1_INVALID_UNIT;
    }
    // save device's status to be used by P1_WaitDevice
    // save abort to be used by P1_WaitDevice
    // V device's semaphore 
    switch(type){
        case USLOSS_CLOCK_DEV:
            if(unit>=USLOSS_CLOCK_UNITS){
                return P1_INVALID_UNIT;
            }
            devices[1].status=status;
            devices[1].abort=abort;
            rc=P1_V(devices[1].sem);
            assert(rc == P1_SUCCESS);
            break;
        case USLOSS_ALARM_DEV:
               if(unit>=USLOSS_ALARM_UNITS){
                return P1_INVALID_UNIT;
            }
            devices[0].status=status;
            devices[0].abort=abort;
            rc=P1_V(devices[0].sem);
            assert(rc == P1_SUCCESS);
            break;
        case USLOSS_DISK_DEV:
            if(unit>=USLOSS_DISK_UNITS){
                return P1_INVALID_UNIT;
            }
            devices[2+unit].status=status;
            devices[2+unit].abort=abort;
            rc=P1_V(devices[2+unit].sem);
            assert(rc == P1_SUCCESS);
            break;
        case USLOSS_TERM_DEV:
            if(unit>=USLOSS_TERM_UNITS){
                return P1_INVALID_UNIT;
            }
            devices[4+unit].status=status;
            devices[4+unit].abort=abort;
            rc=P1_V(devices[4+unit].sem);
            assert(rc == P1_SUCCESS);
            break;
        default:
            return P1_INVALID_TYPE;
    }

    // restore interrupts
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

static void
DeviceHandler(int type, void *arg) 
{
    int status;
    int rc;
    int unit=(int) arg;
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    ticks++;
    rc = USLOSS_DeviceInput(type,unit,&status);
    // if clock device
    //      P1_WakeupDevice every 5 ticks
    //      P1Dispatch(TRUE) every 4 ticks
    // else
    //      P1_WakeupDevice
    if(type==USLOSS_CLOCK_DEV){
        if(ticks%5==0){
            rc = P1_WakeupDevice(type,unit,status,FALSE);
        }
    }else{
        rc = P1_WakeupDevice(type,unit,status,FALSE);
    }

    if(ticks%4==0){
        P1Dispatch(TRUE);
    }
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
    
    int cpid;
    int status;
    while(P1GetChildStatus(0,&cpid,&status)!=P1_NO_CHILDREN){
        USLOSS_WaitInt();
    }

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
