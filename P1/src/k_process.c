/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/02/28
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"

#ifdef DEBUG_0 	
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */

U32 g_switch_flag = 0;          /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
				/* this value will be set by UART handler */


node* next_process_node;

/* process initialization table */
PROC_INIT g_proc_table[NUM_TOTAL_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];
extern PROC_INIT g_kernal_procs[NUM_KERNAL_PROCS];

//linkedList ready_queue;
static linkedList ready_queue[NUM_PRIORITY];
static linkedList block_queue[NUM_PRIORITY];
// Assume: pid mapping
static node node_pool[NUM_TOTAL_PROCS];

static node* node_factory(PCB * pcb){
	// pid mapping 
	node_pool[pcb->m_pid].value = pcb;
	return &node_pool[pcb->m_pid];
}


//Jeff func starts//
int block_queue_empty(){
	int i;
	for ( i = 0; i < NUM_PRIORITY; i++ ){
		//at least one block in it.. not empty
		if(block_queue[i].length != 0){
			return 0
		}
	}
	return 1
}

int ready_queue_empty(){
	int i;
	for ( i = 0; i < NUM_PRIORITY; i++ ){
		//at least one block in it.. not empty
		if(ready_queue[i].length != 0){
			return 0
		}
	}
	return 1
}

/**
 * @brief: Enqueues the PCB into its corresponding queue (based on priority)
 */
void processEnqueue(linkedList[] queue, PCB* thePCB)
{
	node* process_node;
	int priority = thePCB->m_priority; //priority is 0, 1, 2 or 3
	process_node = node_factory(thePCB);
	linkedList.linkedList_push_back(&queue[priority], process_node);
}

/**
 * @brief: Dequeues the first element from the queue with the highest priority
 */
PCB* processDequeue(linkedList[] queue)
{
	int i;
	for ( i = 0; i < NUM_PRIORITY; i++ ){
		//at least one block in it.. not empty
		if(ready_queue[i].length != 0){
			return (PCB*)queue[i].first->value;
		}
	}
	return NULL;
}

/**
 * @brief: Transfers the highest priority PCB from the block queue to the ready queue
 */
void makeReady()
{
	PCB* thePCB = processDequeue(block_queue);
	thePCB->m_state = RDY;
 	processEnqueue(ready_queue,thePCB);
	k_release_processor();
}

//yeah fuck only block your mem hmm?
void makeMeMBlock()
{
	gp_current_process->m_state = MEM_BLOCKED;
	k_release_processor();
}

/**
 * @brief glorified scheduler() and process_switch()
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
  PCB *p_pcb_old = NULL;

  p_pcb_old = gp_current_process;
  gp_current_process = scheduler();

  if ( gp_current_process == NULL  ) {
    gp_current_process = p_pcb_old; // revert back to the old process
    return RTX_ERR;
  }
  if ( p_pcb_old == NULL ) {
    p_pcb_old = gp_current_process;
  }
  process_switch(p_pcb_old);
  return RTX_OK;
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
PCB *scheduler(void)
{
  if (gp_current_process != NULL) {
    if (  is_blocked(gp_current_process) ){
      processEnqueue(block_queue, gp_current_process);
    }
    else {
      processEnqueue(ready_queue, gp_current_process);
    }
  }
  return processDequeue(ready_queue);
}

//returns true if 
int is_blocked (PCB * pcb) {
	if (pcb->m_state == MEM_BLOCKED || pcb->m_state == MSG_BLOCKED){
		return 1;
	}
	else {
		return 0;
	}
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old)
{
  PROC_STATE_E state;

  state = gp_current_process->m_state;

  if (state == NEW) {
    if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
      if (p_pcb_old->m_state != BLK){
        p_pcb_old->m_state = RDY;
      }
      p_pcb_old->mp_sp = (U32 *) __get_MSP();
    }
    gp_current_process->m_state = RUN;
    __set_MSP((U32) gp_current_process->mp_sp);
    __rte();  // pop exception stack frame from the stack for a new processes
  }

  /* The following will only execute if the if block above is FALSE */
  if (gp_current_process != p_pcb_old) {
    if (state == RDY){
      if (p_pcb_old->m_state != BLK){
        p_pcb_old->m_state = RDY;
      }
      p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
      gp_current_process->m_state = RUN;
      __set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack
    } else {
      gp_current_process = p_pcb_old; // revert back to the old proc on error
      return RTX_ERR;
    }
  }
  return RTX_OK;
}



/**
 * @brief: Initializes priority queues, block queues, PCBs, and process tables
 */
void process_init()
{
  int i;
  U32 *sp;

  /* fill out the initialization table */
  set_test_procs();

  g_proc_table[0].m_pid = 0;
  //I KNOW IT IS 4
  g_proc_table[0].m_priority = 4;
  g_proc_table[0].m_stack_size = 0x100;
  g_proc_table[0].mpf_start_pc = &nullProc;

  for ( i = 1; i < NUM_PROCS; i++ ) {
    g_proc_table[i].m_pid = g_test_procs[i-1].m_pid;
    g_proc_table[i].m_priority = g_test_procs[i-1].m_priority;
    g_proc_table[i].m_stack_size = g_test_procs[i-1].m_stack_size;
    g_proc_table[i].mpf_start_pc = g_test_procs[i-1].mpf_start_pc;
  }

  /* initialize exception stack frame (i.e. initial context) for each process */
  for ( i = 0; i < NUM_PROCS; i++ ) {
    int j;
    (gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
    (gp_pcbs[i])->m_state = NEW;
    (gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
    (gp_pcbs[i])->nextPCB = NULL;

    sp = alloc_stack((g_proc_table[i]).m_stack_size);
    *(--sp)  = INITIAL_xPSR;      // user process initial xPSR
    *(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
    for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
      *(--sp) = 0x0;
    }
    (gp_pcbs[i])->mp_sp = sp;
  }


  //This one looks extra for our code
  // for ( i = 0; i < NUM_OF_PRIORITIES; i++ ) {
  //   ReadyPQ[i].head = NULL;
  //   ReadyPQ[i].tail = NULL;
  //   BlockPQ[i].head = NULL;
  //   BlockPQ[i].tail = NULL;
  // }

  /* initialize priority queue */
  for ( i = 0; i < NUM_PROCS; i++ ) {
    //insert PCB[i] into priority queue
#ifdef DEBUG_0
    printf("iValue 0x%x \n", gp_pcbs[i]);
#endif
    processEnqueue(ready_queue, gp_pcbs[i]);
  }
}



/**
 * @brief moves pcb to its correct queue (for the case where a process changes another process' priority)
 */
void moveProcessToPriority(PCB* thePCB, int old_priority) {

  node* process_node;
  int now_priority;
  now_priority = thePCB->m_priority;
  if(now_priority == old_priority){
  	return;//fuck? you really set the same thing? you fucking want me to move you
  	//to the top of your priority?
  }

  //ok.. so you need to move your queue priority to other place...
  process_node = node_factory(thePCB);

  if(is_blocked(thePCB)){
  	linkedList_remove(&block_queue[old_priority], process_node);
  	processEnqueue(block_queue, thePCB);
  }
  else if(thePCB->state == RDY){
  	linkedList_remove(&ready_queue[old_priority], process_node);
  	process_node(ready_queue, thePCB);
  }
  //hmm fuck , you are no in ready or block queue
}

/**
 * @brief Sets process priority, then calls release_processor()
 * @return RTX_ERR on error and RTX_OK on success
 */
int k_set_process_priority(int process_id, int priority){
  int i;
  int old_priority;
  PCB* thePCB;
  if (0 <= priority && priority < NUM_OF_PRIORITIES - 1) {
  	//which is  NUM_OF_PRIORITIES - 1 = 4 
    for (i = 1; i < NUM_PROCS; i++){
      thePCB = gp_pcbs[i];
      if (thePCB->m_pid == process_id){
        #ifdef DEBUG_0
        printf("Setting Process Priority: %d\n", priority);
        #endif /* DEBUG_0 */
        if (thePCB->m_priority != priority) {
          old_priority = thePCB->m_priority;
          thePCB->m_priority = priority;

          if (process_id != gp_current_process->m_pid) {
            moveProcessToPriority(thePCB, old_priority);
          }
          k_release_processor();
        }
        return RTX_OK;
      }
    }
  }

#ifdef DEBUG_0
  printf("Setting process priority failed.");
#endif /* DEBUG_0 */

  return RTX_ERR;
}

/**
 * @brief Gets process priority
 * @return process priority
 */
int k_get_process_priority(int process_id){
  int i;
  int priority = -1;
  for (i = 0; i < NUM_PROCS; i++){
    if (gp_pcbs[i]->m_pid == process_id){
      priority = gp_pcbs[i]->m_priority;
      #ifdef DEBUG_0
      printf("Getting Process Priority: %d\n", priority);
      #endif /* DEBUG_0 */
      return priority;
    }
  }
#ifdef DEBUG_0
  printf("Getting Process Priority: %d\n", priority);
#endif /* DEBUG_0 */
  return priority;
}

//Jeff func ends//
