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
extern PROC_INIT k_test_procs[NUM_KERNEL_PROCS];

//linkedList ready_queue;
static linkedList ready_queue[NUM_PRIORITY];
static linkedList block_queue[NUM_PRIORITY];
// Assume: pid mapping
static node node_pool[NUM_TOTAL_PROCS];

static node* node_factory(PCB * pcb){
	//can't just use m_pority here since all the process has the same prority, 
	//so that it will keep overriding the memory and all the nodes will have the same memory address
	node_pool[pcb->m_pid].value = pcb;
	return &node_pool[pcb->m_pid];
}

// find the first blocked on specific state and put it in ready queue, return 1
// if not found, return 0 
int handle_blocked_process_ready(PROC_STATE_E state){
	//U32 current_prority = gp_current_process->m_priority;
	int i;
	node* temp_node;
	PCB* temp_pcb;
	for(i = 0; i <= NUM_PRIORITY; i++){
		if(block_queue[i].first != NULL){
			// check the whole list and see if there is anything blocked on memory
			temp_node = block_queue[i].first;
			while (temp_node != NULL){
				temp_pcb = (PCB*)temp_node->value;
				if (temp_pcb->m_state == state){
					temp_node = linkedList_remove(&block_queue[i], temp_node->value);
					temp_pcb->m_state = RDY;
					ready_enqueue(temp_pcb);
					return 1;
				}else{
					temp_node = temp_node->next;
				}
			}
		}
	}
	return 0;
}

// find the pcb in the block queue, if not found return 0
// if found but not blocked on message, return 0
// if blocked on message, unblock, put in ready queue and return 1
int handle_msg_blocked_process_ready(PCB* receiver_pcb){
	int i;
	node* temp_node;
	PCB* temp_pcb;
	if (receiver_pcb->m_state != MSG_BLOCKED){
		return 0;
	}
	for(i = 0; i <= NUM_PRIORITY; i++){
		if (linkedList_contain(&block_queue[i], receiver_pcb)){
			temp_node =  linkedList_remove(&block_queue[i], receiver_pcb);
			temp_pcb = (PCB*)temp_node->value;
			temp_pcb->m_state = RDY;
			ready_enqueue(temp_pcb);
			return 1;
		}
	}
	// Shouldn't happen!!!! Blocked on message but not in block queues??????
	printf("Panic! handle_msg_blocked_process_ready: Blocked on message but not in block queues!");
	return 0;
}



PCB * k_get_pcb_from_id (U32 process_id){
	int i;
	for ( i = 0; i < NUM_TOTAL_PROCS; i++ ) {
		if ((gp_pcbs[i])->m_pid == process_id){ //find process with id process_id
			return (gp_pcbs[i]); //return priority of found process
		}
	}
	return NULL; //could not find pcb
}

PCB * k_get_current_process(){
	return gp_current_process;
}

/**
 * Put the PCB into ready queue
*/
void ready_enqueue(PCB * pcb){
	node* process_node;
	//int priority = current_process_node->m_priority;
	int priority = pcb->m_priority;
	//pcb->m_state = RDY;
	//process_node = node_factory(gp_current_process);
	process_node = node_factory(pcb);
	// rpq enqueue(current process) put current process in ready queues
	linkedList_push_back(&ready_queue[priority], process_node); 
}

/**
 * Put the PCB into block queue\

*/
void block_enqueue(PCB * pcb, PROC_STATE_E state){
	node* process_node;
	//int priority = current_process_node->m_priority;
	int priority = pcb->m_priority;
	pcb->m_state = state;
	//process_node = node_factory(gp_current_process);
	process_node = node_factory(pcb);
	// rpq enqueue(current process) put current process in ready queues
	linkedList_push_back(&block_queue[priority], process_node); 
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
	set_kernel_procs();
	set_test_procs();
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i].mpf_start_pc = g_test_procs[i].mpf_start_pc;
		g_proc_table[i].m_priority = g_test_procs[i].m_priority;
		
	}
	
	for(i = NUM_TEST_PROCS; i < NUM_TOTAL_PROCS; i++){
		g_proc_table[i].m_pid = k_test_procs[i-NUM_TEST_PROCS].m_pid;
		g_proc_table[i].m_stack_size = k_test_procs[i-NUM_TEST_PROCS].m_stack_size;
		g_proc_table[i].mpf_start_pc = k_test_procs[i-NUM_TEST_PROCS].mpf_start_pc;
		g_proc_table[i].m_priority = k_test_procs[i-NUM_TEST_PROCS].m_priority;
	}
  
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_TOTAL_PROCS; i++ ) {
		int j;
		gp_pcbs[i]->m_pid = g_proc_table[i].m_pid;
		gp_pcbs[i]->m_state = NEW;
		gp_pcbs[i]->m_priority = g_proc_table[i].m_priority;
		linkedList_init(&gp_pcbs[i]->m_msg_queue);
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
	for ( i = 0; i < NUM_TOTAL_PROCS; i++ ) {
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
	
	//node *blocked_process_node;
	//PCB *blocked_process;
	int i;
	if(gp_current_process != NULL && gp_current_process->m_state != MEM_BLOCKED){
		gp_current_process->m_state = RDY;
		ready_enqueue(gp_current_process);
	} // current process was set to block on memory, should be in block queue
	else if(gp_current_process != NULL && gp_current_process->m_state == MEM_BLOCKED){
		block_enqueue(gp_current_process, MEM_BLOCKED);
	} // current process was set to block on message, should be in block queue
	else if(gp_current_process != NULL && gp_current_process->m_state == MSG_BLOCKED){
		block_enqueue(gp_current_process, MSG_BLOCKED);
	}
	for ( i = 0; i <= NUM_PRIORITY; i++ ){
		if (ready_queue[i].first != NULL){
			// pop off the first ready process with highest priority
			node* firstProcess = linkedList_pop_front(&ready_queue[i]);
			return (PCB *)firstProcess->value;
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
			//ready_enqueue(p_pcb_old);
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
			//ready_enqueue(p_pcb_old);
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack    
		} else {
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		} 
	}else{
			gp_current_process->m_state = RUN;
	}
	return RTX_OK;
}

/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void){
// 1. Set current process to state ready if it's not block
// 2. rpq enqueue(current process) put current process in specific queues
// 3. process switch invokes scheduler and context-switches to the new process
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

int k_set_process_priority(int process_id, int priority){
	
	int i;
	U32 old_priority;
	PCB* proc_to_set_priority;
	if (process_id == 0 || priority == 4){ //NULL PROCESS PRIORITY CANNOT BE CHANGED
		return RTX_ERR;
	}
	for ( i = 0; i < NUM_TOTAL_PROCS; i++ ) {
		if ((gp_pcbs[i])->m_pid == process_id){ //find process with id process_id
			proc_to_set_priority = gp_pcbs[i];
			old_priority = proc_to_set_priority->m_priority;
			proc_to_set_priority ->m_priority = priority; //update priority of found process
			if(proc_to_set_priority != gp_current_process){
				switch(proc_to_set_priority->m_state) 
				{
					case RDY:
						linkedList_remove(&ready_queue[old_priority], proc_to_set_priority);
						ready_enqueue(proc_to_set_priority);
						break;
					case MEM_BLOCKED:
					case MSG_BLOCKED:
						linkedList_remove(&block_queue[old_priority], proc_to_set_priority);
						block_enqueue(proc_to_set_priority, proc_to_set_priority->m_state);
						break;
					default :
						break;
				}
			}		
		}
	}
	return k_release_processor();  //process not found
}

int get_process_priority(int process_id){
	int i;
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		if ((gp_pcbs[i])->m_pid == process_id){ //find process with id process_id
			return (gp_pcbs[i])->m_priority; //return priority of found process
		}
	}
	return -1; //process not found
}

// typedef enum {
// 	PRINT_READY=0,
// 	PRINT_MEM_BLOCKED,
// 	PRINT_MSG_BLOCKED
// }PROCESS_QUEUE_ID

void print_queue(PROCESS_QUEUE_ID id){
	int i;
	node* temp_node;
	PCB* temp_pcb;
	PROC_STATE_E state;
	switch(id){
		case PRINT_READY:
			state = RDY;
			printf("Printing ready queue...\r\n");
			break;
		case PRINT_MEM_BLOCKED:
			state = MEM_BLOCKED;
			printf("Printing blocked on memory queue...\r\n");
			break;
		case PRINT_MSG_BLOCKED:
			state = MSG_BLOCKED;
			printf("Printing blocked on message queue...\r\n");
			break;
		default:
			printf("INVALID ID TO PRINT QUEUE\r\n");
			return;
	}

	if (id == PRINT_READY){ // ready queue
		for(i = 0; i <= NUM_PRIORITY; i++){
			printf("Priority: %d\r\n", i);
			if(ready_queue[i].first != NULL){
				temp_node = block_queue[i].first;
				while (temp_node != NULL){
					temp_pcb = (PCB*)temp_node->value;
					if (temp_pcb->m_state == state){
						printf("pid: %d  ", temp_pcb->m_pid);
					}else if( temp_pcb->m_state == MEM_BLOCKED || temp_pcb->m_state == MSG_BLOCKED ){
						printf("WTF?!?!?!?!?!?!?! Blocked PCB in ready queue!");
					}
					temp_node = temp_node->next;
				}
			}
			printf("\r\n");
		}
	}else{ // block queue
		for(i = 0; i <= NUM_PRIORITY; i++){
			printf("Priority: %d\r\n", i);
			if(block_queue[i].first != NULL){
				temp_node = block_queue[i].first;
				while (temp_node != NULL){
					temp_pcb = (PCB*)temp_node->value;
					if (temp_pcb->m_state == state){
						printf("pid: %d  ", temp_pcb->m_pid);
					}else if( temp_pcb->m_state == RDY){
						printf("WTF?!?!?!?!?!?!?! Ready PCB in block queue!");
					}
					temp_node = temp_node->next;
				}
			}
			printf("\r\n");
		}
	}

	

}

void print_current_process(){
	printf("Current process: %d\r\n", gp_current_process->m_pid);
}
