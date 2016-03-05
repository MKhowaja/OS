/* @brief: rtx.h User API prototype, this is only an example
 * @author: Yiqing Huang
 * @date: 2014/01/17
 */
#ifndef RTX_H_
#define RTX_H_

/* ----- Definitations ----- */
#define RTX_ERR -1
#define NULL 0

#define NUM_TEST_PROCS 6
//#define NUM_TOTAL_PROCS 7

/* Process Priority. The bigger the number is, the lower the priority is*/
/*#define HIGHEST 0
#define HIGH    1
#define MEDIUM  2
#define LOW     3
#define LOWEST  4
*/

#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3

/* Process IDs */
#define PID_NULL 					0	
#define PID_CLOCK  				1
#define PID_UART_IPROC   	2
#define PID_KCD          	3
#define PID_CRT          	4


#define PID_P1   5
#define PID_P2   6
#define PID_P3   7
#define PID_P4   8
#define PID_P5   9
#define PID_P6   10
#define PID_A    11
#define PID_B    12
#define PID_C    13

#define DEFAULT 0
#define KCD_REG 1
#define CRT_DIS 2



#define PROC_BLK_SIZE 0x100 //size of each process' stack
 
/* ----- Types ----- */
typedef unsigned int U32;

/* initialization table item */
#ifndef PROC_INIT_STRUCT
#define PROC_INIT_STRUCT
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority, not used in this example. */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */    
} PROC_INIT;
#endif


/* user version */
#ifndef MSG_BUF
#define MSG_BUF
typedef struct msgbuf
{
	int msg_type;
	char mText[1];
} MSGBUF;
#endif

/* ----- RTX User API ----- */
#define __SVC_0  __svc_indirect(0)

extern void k_rtx_init(void);
#define rtx_init() _rtx_init((U32)k_rtx_init)
extern void __SVC_0 _rtx_init(U32 p_func);

extern int k_release_processor(void);
#define release_processor() _release_processor((U32)k_release_processor)
extern int __SVC_0 _release_processor(U32 p_func);

extern int k_get_process_priority(int pid);
#define get_process_priority(pid) _get_process_priority((U32)k_get_process_priority, pid)
extern int _get_process_priority(U32 p_func, int pid) __SVC_0;
/* __SVC_0 can also be put at the end of the function declaration */

extern int k_set_process_priority(int pid, int prio);
#define set_process_priority(pid, prio) _set_process_priority((U32)k_set_process_priority, pid, prio)
extern int _set_process_priority(U32 p_func, int pid, int prio) __SVC_0;

extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;
/* __SVC_0 can also be put at the end of the function declaration */

extern int k_release_memory_block(void *);
#define release_memory_block(p_mem_blk) _release_memory_block((U32)k_release_memory_block, p_mem_blk)
extern int _release_memory_block(U32 p_func, void *p_mem_blk) __SVC_0;

/* IPC Management */
extern int k_send_message(int pid, void *p_msg);
#define send_message(pid, p_msg) _send_message((U32)k_send_message, pid, p_msg)
extern int _send_message(U32 p_func, int pid, void *p_msg) __SVC_0;

extern void *k_receive_message(int *p_pid);
#define receive_message(p_pid) _receive_message((U32)k_receive_message, p_pid)
extern void *_receive_message(U32 p_func, void *p_pid) __SVC_0;

/* Timing Service */
// extern int k_delayed_send(int pid, void *p_msg, int delay);
// #define delayed_send(pid, p_msg, delay) _delayed_send((U32)k_delayed_send, pid, p_msg, delay)
// extern int _delayed_send(U32 p_func, int pid, void *p_msg, int delay) __SVC_0;  
// #endif /* !RTX_H_ */


#endif /* !RTX_H_ */
