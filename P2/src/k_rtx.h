/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_
#include "common.h"
/*----- Definitations -----*/

// #define RTX_ERR -1
// #define RTX_OK  0

// #define NULL 0

// #define NUM_KERNEL_PROCS 5
// #define NUM_TEST_PROCS 6
// #define NUM_TOTAL_PROCS (NUM_KERNEL_PROCS+NUM_TEST_PROCS)


#define KEY_READY '!'
#define KEY_BLOCKED_MEM ','
#define KEY_BLOCKED_MSG '.'
#define KEY_MSG_LOG '/'

#define NUM_MSG_BUFFERED 10


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

typedef enum {
	PRINT_READY=0,
	PRINT_MEM_BLOCKED,
	PRINT_MSG_BLOCKED
}PROCESS_QUEUE_ID;

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
	//linkedList m_msg_queue;
	MSG_BUF* msg_queue;
	//stack size?
	//whether process is i-process?
} PCB;

/* user version */
// #ifndef MSG_BUF
// #define MSG_BUF

// //typedef struct msgbuf
// //{
// //	int mtype;
// //	char mtext[1];
// //} MSG_BUF;


// /* kernel version */
// typedef struct msg_t
// {
// 	int sender_pid;				 sender process id
// 	int receiver_pid;			/* receiver process id */
// 	int msg_delay;				/* message delay */
// 	int mtype;					/* message type */
// 	char mtext[5];				/* message data */
// } MSG_BUF;
// #endif



/* log buffer version */
typedef struct log_msg_t
{
	int sender_pid;				/* sender process id*/
	int receiver_pid;			/* receiver process id */
	int mtype;				/* message type */
	U32 timestamp;				/* The time stamp of the transaction (using the RTX clock) */
	char mtext[17];				/* message data */
} LOG_MSG_BUF;

#define SZ_MEM_BLK 128           /* fixed size of memory block 128B default */
//#define SZ_MEM_BLK_WITH_HEADER SZ_MEM_BLK+0x20
//#define NUM_MEM 20
#define NUM_MEM 2
#define PROC_BLK_SIZE 0x100
#endif // ! K_RTX_H_
