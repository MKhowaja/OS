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
void check_preemption(void);
void block_enqueue(PCB * pcb, PROC_STATE_E state);
void ready_enqueue(PCB * pcb);
int is_blocked (PCB * pcb);

int k_set_process_priority(int process_id, int priority);
int k_get_process_priority(int process_id);

extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */
extern void set_kernal_procs(void);      /* kernal process initial set up */
extern int has_free_memory(void);


#endif /* ! K_PROCESS_H_ */
