/**
 * @file:   k_process.h
 * @brief:  process management hearder file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef K_PROCESS_H_
#define K_PROCESS_H_

#include "k_rtx.h"

/* ----- Definitions ----- */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

/* ----- Functions ----- */

void process_init(void);               /* initialize all procs in the system */
PCB *scheduler(void);                  /* pick the pid of the next to run process */
int k_release_processor(void);           /* kernel release_process function */

int k_set_process_priority(int process_id, int priority);
int k_get_process_priority(int process_id);

PCB * k_get_pcb_from_id (U32 process_id);
PCB * k_get_current_process(void);

void ready_enqueue(PCB * pcb);
void block_enqueue(PCB * pcb, PROC_STATE_E state);

extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */
extern void set_kernel_procs(void);      /* kernel process initial set up */

void print_queue(PROCESS_QUEUE_ID id);  	/* Print ready or block queue */
void print_current_process(void);				/* Print current process */


int handle_blocked_process_ready(PROC_STATE_E state);
int handle_msg_blocked_process_ready(PCB* receiver_pcb);
#endif /* ! K_PROCESS_H_ */