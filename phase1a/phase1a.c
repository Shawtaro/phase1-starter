#include "phase1Int.h"
#include "usloss.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

extern  USLOSS_PTE  *P3_AllocatePageTable(int pid);
extern  void        P3_FreePageTable(int pid);

typedef struct Context {
    void            (*startFunc)(void *);
    void            *startArg;
    USLOSS_Context  context;
    // you'll need more stuff here
    int isUsed;
    char *stack;
} Context;

static Context contexts[P1_MAXPROC];

static int currentCid = -1;

/*
 * Helper function to call func passed to P1ContextCreate with its arg.
 */
static void launch(void)
{
    assert(contexts[currentCid].startFunc != NULL);
    contexts[currentCid].startFunc(contexts[currentCid].startArg);
}

void P1ContextInit(void)
{
	// Only Kernel can rnable/disbale interrupts
	// First bit corresponds with current mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		USLOSS_Console("User mode\n");
		USLOSS_Halt(1);
    } 
    // initialize contexts
    for (int i = 0; i < P1_MAXPROC; ++i)
    {
        contexts[i].startFunc=NULL;
        contexts[i].startArg=NULL;
        contexts[i].isUsed=0;
    }
}

int P1ContextCreate(void (*func)(void *), void *arg, int stacksize, int *cid) {
	// Only Kernel can rnable/disbale interrupts
	// First bit corresponds with current mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		USLOSS_Console("User mode\n");
		USLOSS_Halt(1);
    } 
    int result = P1_SUCCESS;
    // check whether the stacksize is legal
    if(stacksize<USLOSS_MIN_STACK){
        return P1_INVALID_STACK;
    }
    // find a free context and initialize it
    // allocate the stack, specify the startFunc, etc.
    int index;
    for (index = 0; index < P1_MAXPROC; index++)
    {
        if(contexts[index].isUsed==0){
            *cid=index;
            contexts[index].isUsed=1;
            contexts[index].startFunc=func;
            contexts[index].startArg=arg;
            contexts[index].stack=malloc(stacksize);
            USLOSS_ContextInit(&contexts[index].context,contexts[index].stack,stacksize,P3_AllocatePageTable(index),launch);
            break;
        }
    }
    // too many contexts
    if(index==P1_MAXPROC){
        return P1_TOO_MANY_CONTEXTS;
    }
    return result;
}

int P1ContextSwitch(int cid) {
	// Only Kernel can rnable/disbale interrupts
	// First bit corresponds with current mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		USLOSS_Console("User mode\n");
		USLOSS_Halt(1);
    } 
    int result = P1_SUCCESS;
    // check whether cid is valid
    if(cid<0||cid>P1_MAXPROC){
        return P1_INVALID_CID;
    }
    // switch to the specified context
    currentCid=cid;
    USLOSS_ContextSwitch(NULL,&contexts[cid].context);
    return result;
}

int P1ContextFree(int cid) {
	// Only Kernel can rnable/disbale interrupts
	// First bit corresponds with current mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		USLOSS_Console("User mode\n");
		USLOSS_Halt(1);
    } 
    int result = P1_SUCCESS;
    // check whether the cid is valid
    if(cid<0||cid>P1_MAXPROC){
        return P1_INVALID_CID;
    }
    // free the stack and mark the context as unused
    contexts[cid].startFunc=NULL;
    contexts[cid].startArg=NULL;
    P3_FreePageTable(cid);
    free(contexts[cid].stack);
    contexts[cid].stack=NULL;
    contexts[cid].isUsed=0;
    return result;
}


void P1EnableInterrupts(void) {
	// Only Kernel can enable/disbale interrupts
	// First bit corresponds with current mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		USLOSS_Console("User mode\n");
		USLOSS_Halt(1);
    }
	// clear the interrupt bit in the PSR
	USLOSS_PsrSet(USLOSS_PsrGet() | USLOSS_PSR_CURRENT_INT);
}

/*
 * Returns true if interrupts were enabled, false otherwise.
 */
int P1DisableInterrupts(void) {
    int enabled = FALSE;
	// Only Kernel can enable/disbale interrupts
	// First bit corresponds with current mode
	if ((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) == 0){
		printf("User mode\n");
		USLOSS_Halt(1);
    } 
    // set enabled to TRUE if interrupts are already enabled
	enabled=((USLOSS_PsrGet() & USLOSS_PSR_CURRENT_INT)!=0);
	// set the interrupt bit in the PSR
	USLOSS_PsrSet(USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_INT);
	
    return enabled;
}
