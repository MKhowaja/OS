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

/**
 * Put the PCB into ready queue
*/
void ready_enqueue(PCB * pcb){
	node* process_node;
	int priority = pcb->m_priority;
	//pcb->m_state = RDY;
	process_node = node_factory(pcb);
	linkedList_push_back(&ready_queue[priority], process_node); 
}

/**
 * Put the PCB into block queue\
*/
void block_enqueue(PCB * pcb, PROC_STATE_E state){
	node* process_node;
	int priority = pcb->m_priority;
	pcb->m_state = state;
	process_node = node_factory(pcb);
	linkedList_push_back(&block_queue[priority], process_node); 
}

void check_preemption(){
	int i;
	node* blocked_process_node;
	PCB* blocked_process;

	for ( i = 0; i < NUM_PRIORITY; i++ ){
		// if memory is available, unblock the highest priority	
		if(has_free_memory() == 1 && block_queue[i].first != NULL){
			blocked_process_node = linkedList_pop_front(&block_queue[i]);
			blocked_process = (PCB *)blocked_process_node->value;
			blocked_process->m_state = RDY;
			linkedList_push_back(&ready_queue[i],blocked_process_node);
			// preempt the current process if the priority is higher
			if (blocked_process->m_priority > gp_current_process->m_priority){
				k_release_processor();
			}
			break;
		}
		// check if there are ready process that has higher priority
		if ( i > gp_current_process->m_priority && ready_queue[i].first != NULL){
			k_release_processor();
			break;
		}
	}
}

/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init() 
{
	int i;
	U32 *sp;
  
    /* fill out the initialization table */
    set_kernal_procs();
	set_test_procs();
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
		
	}

	for ( i = NUM_TEST_PROCS; i < NUM_TOTAL_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_kernal_procs[i-NUM_TEST_PROCS].m_pid;
		g_proc_table[i].m_stack_size = g_kernal_procs[i-NUM_TEST_PROCS].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_kernal_procs[i-NUM_TEST_PROCS].mpf_start_pc;
		g_proc_table[i].m_priority = g_kernal_procs[i-NUM_TEST_PROCS].m_priority;
	}
  
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
	}
	//Initialize ready queue and block queue
	for ( i = 0; i < NUM_PRIORITY; i++ ){
		linkedList_init(&ready_queue[i]);
		linkedList_init(&block_queue[i]);
	}
	// Initialize everything to ready queue
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		ready_enqueue(gp_pcbs[i]);
	}
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to nullProc.
 *      No other effect on other global variables.
 */
PCB *scheduler(void){
	int i;
	node* first_process_node;
	PCB* first_process;

	for ( i = 0; i < NUM_PRIORITY; i++ ){
		if (ready_queue[i].first != NULL){
			// pop off the first ready process with highest priority
			first_process_node = ready_queue[i].first;
			first_process = (PCB *) first_process_node->value;
			if(gp_current_process == NULL){
				linkedList_pop_front(&ready_queue[i]);
				return first_process;
			}
			if (first_process->m_priority <= gp_current_process->m_priority){
			// return new process only when the priority is at least the same level
				first_process_node = linkedList_pop_front(&ready_queue[i]);
				return first_process;
			}else{
				return gp_current_process;
			}	
		}
	}
	// Error
	return NULL;
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
			p_pcb_old->m_state = RDY;
			ready_enqueue(p_pcb_old);
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	}
	
	/* The following will only execute if the if block above is FALSE */

	if (gp_current_process != p_pcb_old) {
		if (state == RDY){ 		
			p_pcb_old->m_state = RDY; 
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			gp_current_process->m_state = RUN;
			ready_enqueue(p_pcb_old);
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack    
		} else {
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		} 
	}
	return RTX_OK;
}

/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void){
// 1. Set current process to state ready
// 2. rpq enqueue(current process) put current process in ready queues
// 3. process switch invokes scheduler and context-switches to the new process
	PCB *p_pcb_old = NULL;
	p_pcb_old = gp_current_process;

	//if (gp_current_process!= NULL){
		//gp_current_process->mp_sp = (U32 *) __get_MSP();
		//ready_enqueue(gp_current_process);
	//}	
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

int k_set_process_priority(int process_id, int priority){
	
	int i;
	if (process_id == 0){ //NULL PROCESS PRIORITY CANNOT BE CHANGED
		return RTX_ERR;
	}
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		if ((gp_pcbs[i])->m_pid == process_id){ //find process with id process_id
			(gp_pcbs[i])->m_priority = priority; //update priority of found process
			check_preemption();
			return RTX_OK;
		}
	}
	return RTX_ERR;  //process not found
}

int k_get_process_priority(int process_id){
	int i;
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		if ((gp_pcbs[i])->m_pid == process_id){ //find process with id process_id
			return (gp_pcbs[i])->m_priority; //return priority of found process
		}
	}
	return RTX_ERR; //process not found
}
