#include "k_rtx.h"
#include "uart_polling.h"
#include "k_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_kernal_procs[NUM_KERNAL_PROCS];


void set_kernal_procs() {
	// initilize the null process
	g_kernal_procs[0].m_pid = 0;
	g_kernal_procs[0].m_stack_size = PROC_BLK_SIZE;
	g_kernal_procs[0].mpf_start_pc = &nullProc;
	g_kernal_procs[0].m_priority = NULLPROC;
}

void nullProc (void){
	int ret_val;
	printf("Null Process Start\n");
	while (1){
		printf("while loop start");
		ret_val = k_release_processor();
		#ifdef DEBUG_0
				printf("nullProc: ret_val=%d\n", ret_val);
		#endif /* DEBUG_0 */
	}
}
