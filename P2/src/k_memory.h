/**
 * @file:   k_memory.h
 * @brief:  kernel memory managment header file
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_MEM_H_
#define K_MEM_H_

#include "k_rtx.h"


/* ----- Definitions ----- */
#define RAM_END_ADDR 0x10008000
typedef struct MemNode MemNode;

struct MemNode {
	MemNode* next;
};

typedef struct MemList MemList;
struct MemList{
	MemNode* first;
	MemNode* last;
};

/* ----- Variables ----- */
/* This symbol is defined in the scatter file (see RVCT Linker User Guide) */  
extern unsigned int Image$$RW_IRAM1$$ZI$$Limit; 
extern PCB **gp_pcbs;
//extern PROC_INIT g_proc_table[NUM_TOTAL_PROCS];

extern int handle_blocked_process_ready(PROC_STATE_E state);
extern int k_release_processor(void);
extern void block_enqueue(PCB * pcb, PROC_STATE_E state);

/* ----- Functions ------ */
void memory_init(void);
U32 *alloc_stack(U32 size_b);
void *k_request_memory_block(void);
int k_release_memory_block(void *);

#endif /* ! K_MEM_H_ */
