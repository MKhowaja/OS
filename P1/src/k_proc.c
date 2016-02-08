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
