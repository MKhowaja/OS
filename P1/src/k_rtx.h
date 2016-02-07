/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_

/*----- Definitations -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0

#define NUM_KERNEL_PROCS 1
#define NUM_TEST_PROCS 6
#define NUM_TOTAL_PROCS 7

//#define LOWEST 4
//#define PROC_BLK_SIZE 0x100 //size of each process' stack


/*Added just in case since i don't know if vera actually needs it,
it is used in k_process.c*/
#define NUM_PRIORITY 5

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */
#include "list.h"

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states, note we only assume three states in this example */
typedef enum {
	NEW = 0,
	RDY,
	RUN,
    MEM_BLOCKED, //blocked on resource
    MSG_BLOCKED, //waiting on message
    INTERRUPTED,
    EXIT
} PROC_STATE_E;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb 
{ 
	//struct pcb *mp_next;  /* next pcb, not used in this example */  
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	int m_priority;
	PROC_STATE_E m_state;   /* state of the process */
	//stack size?
	//whether process is i-process?
} PCB;

/* initialization table item */
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority, not used in this example. */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */    
} PROC_INIT;

#define SZ_MEM_BLK 0x400           /* fixed size of memory block 128B default */
#define SZ_MEM_BLK_WITH_HEADER SZ_MEM_BLK+0x20
//#define NUM_MEM 20
#define NUM_MEM 2
#define PROC_BLK_SIZE 0x100
#endif // ! K_RTX_H_
