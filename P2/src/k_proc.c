/**
 * @file:   kernel_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "k_proc.h"
#include "timer.h"
#include "k_message.h"
#include "k_rtx.h"
#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT k_test_procs[NUM_KERNEL_PROCS];

void set_kernel_procs() {
	k_test_procs[0].m_pid = 0;
	k_test_procs[0].m_stack_size = 0X100;
	k_test_procs[0].mpf_start_pc = &nullProc;
	k_test_procs[0].m_priority = 4;//LOWEST;
	
	k_test_procs[0].mpf_start_pc = &nullProc;
	//add timer i process to k_test_procs

	k_test_procs[1].m_pid = 1;
	k_test_procs[1].m_stack_size = 0X100;
	k_test_procs[1].mpf_start_pc = &timer_i_process;
	k_test_procs[1].m_priority = 4;
}

void nullProc (void){
	int ret_val;
	printf("Null Process Start\n");
	while (1){
		printf("while loop start");
		ret_val = release_processor();
		#ifdef DEBUG_0
				printf("nullProc: ret_val=%d\n", ret_val);
		#endif /* DEBUG_0 */
	}
}

//every keystroke
// 1.respond msg from crt, -> serial port 
// 2.get character, forward to kcd
void uart_i_process(){

}


//response to crt 
void crt(void){
	MSG_T* msg;

	while(1){
		msg = k_receive_message(NULL);
		if(msg->msg_type == CRT_DIS){
			//send msg to uart_i_process
			//we need id of uart_i_process
			// send_message( pid , msg);

			
		}
		else{
			//do nothing except releasing msg block	
			k_release_memory_block(msg);
		}
	}
}

