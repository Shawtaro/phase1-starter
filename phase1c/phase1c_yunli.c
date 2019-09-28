
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include "usloss.h"
#include "phase1Int.h"

typedef struct Sem
{
    char        name[P1_MAXNAME+1];
    u_int       value;
    // more fields here
} Sem;

static Sem sems[P1_MAXSEM];

void 
P1SemInit(void) 
{
    P1ProcInit();
    // initialize sems here
    for (int i = 0; i < P1_MAXSEM; ++i){
        sems[i].value=-1;
        sems[i].name[0]='\0';
    }
}

int P1_SemCreate(char *name, unsigned int value, int *sid)
{
    int result = P1_SUCCESS;
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // disable interrupts
    int prvInterrupt=P1DisableInterrupts();
    // check parameters
    if(name==NULL){
        return P1_NAME_IS_NULL;
    }
    if(strlen(name)>P1_MAXNAME){
        return P1_NAME_TOO_LONG;
    }
    for (int i = 0; i < P1_MAXSEM; ++i){
        if(strcmp(sems[i].name,name)==0){
            return P1_DUPLICATE_NAME;
        }
    }
    // find a free Sem and initialize it
    for (*sid=0; *sid< P1_MAXSEM; (*sid)++){
        if (sems[*sid].name[0]=='\0'){
            break;
        }
    }
    if (*sid >= P1_MAXSEM){
        return P1_TOO_MANY_SEMS;
    }
    sems[*sid].value=value;
    strcpy(sems[*sid].name,name);
    // re-enable interrupts if they were previously enabled
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

int P1_SemFree(int sid) 
{
    int     result = P1_SUCCESS;
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // check sid
    if(sid<0||sid>=P1_MAXSEM||sems[sid].name[0]=='\0'){
        return P1_INVALID_SID;
    }
    // check block
    for (int i = 0; i < P1_MAXPROC; ++i){
        P1_ProcInfo *info = malloc(sizeof(P1_ProcInfo));
        if(P1_GetProcInfo(i,info)==P1_SUCCESS){
            if (info->sid == sid && info->state==P1_STATE_BLOCKED){
                return P1_BLOCKED_PROCESSES;
            }
        }
        free(info);
    }
    sems[sid].name[0]='\0';
    sems[sid].value=-1;
    return result;
}

int P1_P(int sid) 
{
    int result = P1_SUCCESS;
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    if(sid<0||sid>=P1_MAXSEM||sems[sid].name[0]=='\0'){
        return P1_INVALID_SID;
    }
    // disable interrupts
    int prvInterrupt=P1DisableInterrupts();

    // while value == 0
    //      set state to P1_STATE_BLOCKED
    int pid=P1_GetPid();
    while(sems[sid].value == 0){
        P1SetState(pid,P1_STATE_BLOCKED,sid);
        P1Dispatch(FALSE);
    }
    // value--
    sems[sid].value--;
    // re-enable interrupts if they were previously enabled
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

int P1_V(int sid) 
{
    int result = P1_SUCCESS;
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // check for sid
    if(sid<0||sid>=P1_MAXSEM||sems[sid].name[0]=='\0'){
        return P1_INVALID_SID;
    }
    // disable interrupts
    int prvInterrupt=P1DisableInterrupts();

    // value++
    sems[sid].value++;
    // if a process is waiting for this semaphore
    //      set the process's state to P1_STATE_READY
    for (int i = 0; i < P1_MAXPROC; ++i)
    {
        P1_ProcInfo *info = malloc(sizeof(P1_ProcInfo));
        if(P1_GetProcInfo(i,info)==P1_SUCCESS){
            if (info->sid == sid){
                P1SetState(i,P1_STATE_READY,sid);
            }
        }
        free(info);
    }
    // re-enable interrupts if they were previously enabled
    if (prvInterrupt==1){
        P1EnableInterrupts();
    }
    return result;
}

int P1_SemName(int sid, char *name) {
    int result = P1_SUCCESS;
    // check for kernel mode
    if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
        USLOSS_IllegalInstruction();
    }
    // check for sid
    if(sid<0||sid>=P1_MAXSEM||sems[sid].name[0]=='\0'){
        return P1_INVALID_SID;
    }
    if(sems[sid].name[0]=='\0'){
        return P1_NAME_IS_NULL;
    }
    strcpy(name,sems[sid].name);
    return result;
}

