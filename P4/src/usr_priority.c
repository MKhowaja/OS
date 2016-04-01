#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "string.h"
#include <stdlib.h>

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
int passed_test[NUM_TEST_PROCS];
int isFinished = 0;

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		//g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=0x200;
		passed_test[i] = 0;
	}
	//passed_test[5] = 1;
  
	g_test_procs[0].m_pid = PID_SET_PRIO;
	g_test_procs[0].mpf_start_pc = &set_process_proc;
	g_test_procs[0].m_priority   = HIGH;
    
    g_test_procs[1].m_pid = PID_P1;
	g_test_procs[1].mpf_start_pc = &test_proc_for_set_process_proc;
	g_test_procs[1].m_priority   = LOWEST;
	/*
	g_test_procs[0].m_pid = PID_P1;  ;
	g_test_procs[0].mpf_start_pc = &proc_p2_1;
	g_test_procs[0].m_priority   = HIGH;
	
	g_test_procs[1].m_pid = PID_P2;
	g_test_procs[1].mpf_start_pc = &proc_p2_2;
	g_test_procs[1].m_priority   = MEDIUM; 
	
	g_test_procs[2].m_pid = PID_P3;
	g_test_procs[2].mpf_start_pc = &proc_p2_3;
	g_test_procs[2].m_priority   = MEDIUM;
	
	g_test_procs[3].m_pid = PID_P4;
	g_test_procs[3].mpf_start_pc = &proc_p2_4;
	g_test_procs[3].m_priority   = MEDIUM;
	
	g_test_procs[4].m_pid = PID_P5;
	g_test_procs[4].mpf_start_pc = &proc_p2_5;
	g_test_procs[4].m_priority   = MEDIUM;
	
	g_test_procs[5].m_pid= PID_P6;
	g_test_procs[5].mpf_start_pc = &proc_p2_6;
	g_test_procs[5].m_priority   = LOWEST;
	*/
	
	uart1_init();
}

void set_process_proc(void){
	int i;
	int pid;
    int space_loc;
    int priority;
	MSG_BUF* read_msg;
    MSG_BUF* msg;//send out
	int sender_id;
	const int buf_size = 100;
	char buf[buf_size];
	char cmd[3] = "%C";
    char buf_to_int [3];

    msg = (MSG_BUF*)request_memory_block();
    //reg cmd
    msg->mtype = KCD_REG;
    strncpy(msg->mtext, cmd, strlen(cmd)+1);
    send_message(PID_KCD, msg);
    while (1){
        for(i = 0; i < buf_size; i++){
            buf[i] = '\0';
        }
        read_msg = receive_message(&sender_id);
		strncpy(buf, read_msg->mtext, buf_size);
		release_memory_block(read_msg);
		if( strlen(buf) < 8 || buf[0] != cmd[0] ||  buf[1] != cmd[1] || buf[2] != ' '){
            //do nothing, next
            #ifdef DEBUG_0
				printf("ERROR! Set Process Proc got invalid message\r\n");
			#endif /* DEBUG_0 */
			continue;
		}
        if (buf[4] == ' '){
            space_loc = 4;
            if (buf[3] < 48 || buf[3] > 57) { //NaN
                #ifdef DEBUG_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[0] = buf[3];
            buf_to_int[1] = '\0';
        }
        else {
            space_loc = 5;
            if (buf[3] < 48 || buf[3] > 57) { //NaN
                #ifdef DEBUG_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[0] = buf[3];
            if (buf[4] < 48 || buf[4] > 57) { //NaN
                #ifdef DEBUG_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[1] = buf[4];
            buf_to_int[2] = '\0';
        }
        if (space_loc+2 != strlen(buf)-2){
            #ifdef DEBUG_0
				printf("ERROR! Set Process Proc got invalid chars\r\n");
			#endif /* DEBUG_0 */
			continue;
        }
        pid = atoi(buf_to_int);
        if (pid < 1 || pid > 13){ //should not allow change priority of NULL or I PROCS
            #ifdef DEBUG_0
				printf("ERROR! Set Process Proc got restricted pid\r\n");
			#endif /* DEBUG_0 */
			continue;
        }
        if (buf[space_loc] != ' '){
            #ifdef DEBUG_0
				printf("ERROR! Set Process Proc got invalid message\r\n");
			#endif /* DEBUG_0 */
			continue;
        }
        if (buf[space_loc+1] < 48 || buf[space_loc+1] > 57) { //NaN
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got NaN for Priority\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        buf_to_int[0] = buf[space_loc + 1];
        buf_to_int[1] = '\0';
        priority = atoi(buf_to_int);
        if (priority < 0 || priority > 3){
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got Invalid Priority\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        set_process_priority (pid, priority);
	}
}

void test_proc_for_set_process_proc(void){
    int i;
    while (1){
        if (i%20 == 0){
            #ifdef DEBUG_0
                printf("Test Proc has Priority: %d\r\n", get_process_priority(PID_P1));
            #endif /* DEBUG_0 */
        }
        i++;
        i%=20;
    }
}
