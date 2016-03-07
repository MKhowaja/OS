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
	passed_test[5] = 1;	
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
	
	//g_test_procs[6].m_pid = PID_CLOCK;
  //g_test_procs[6].m_priority = HIGH;
  //g_test_procs[6].mpf_start_pc = &clock_proc;
	
	uart1_init();
}

/*
proc1:
HIGH
send msg ti proc2
receive msg from proc#
get blocked until get message
release msg
set priority to lowest
context switch
*/

void proc_p2_1(void) {
	MSG_BUF *msg = NULL;
	char send_msg[6] = "Blah1";
	char receive_msg[6];
//	char* endptr;
	int sender_id;
	
	//#ifdef DEBUG_0
		 //printf("%ld\n", strtoimax(" -123junk",&endptr,10));
	//#endif /* DEBUG_0 */
	
	msg = (MSG_BUF *) request_memory_block();
	strncpy(msg->mtext, send_msg, strlen(send_msg)+1);
	msg->mtype = DEFAULT;
	
	#ifdef DEBUG_0
		printf("proc2 send message to proc %d : %s" , PID_P3, msg->mtext);
	#endif /* DEBUG_0 */
	send_message(PID_P3,msg);
	
	msg = (MSG_BUF*) receive_message(&sender_id);
	#ifdef DEBUG_0
		printf("proc2 got message from proc %d : %s",(sender_id), msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg, msg->mtext, strlen(msg->mtext)+1);
	
	release_memory_block(msg);
	msg = NULL;
	set_process_priority(PID_P1, LOWEST);
	if(strcmp(receive_msg,"Blah4") == 0 && sender_id == PID_P4){
			passed_test[0] = 1;
	}
	
	while(1) {
		uart1_put_string("proc1: \r\n");
		release_processor();
	}
	
}

/*
Proc2:
MEDIUM
use delay_send to itself
delay 1 second
block since try to recieve after delay send
*/
void proc_p2_2(void) {

	MSG_BUF *msg = NULL;
	char send_msg[6] = "Blah2";
	char receive_msg[6];
	int sender_id;
	
	msg = (MSG_BUF *) request_memory_block();
	strncpy(msg->mtext,send_msg,strlen(send_msg)+1);
	msg->mtype = DEFAULT;
	
	delayed_send(PID_P2,msg,1000);
	#ifdef DEBUG_0
		printf("proc2 send message to itself: %s", msg->mtext);
	#endif /* DEBUG_0 */
	
	msg = receive_message(&sender_id);
	#ifdef DEBUG_0
		printf("proc2 got message from itself: %s", msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg,msg->mtext,strlen(msg->mtext)+1);
	release_memory_block(msg);
	
	set_process_priority(PID_P2, LOWEST);

	if(strcmp(receive_msg,"Blah2") == 0 && sender_id == PID_P2){
			passed_test[1] = 1;
			isFinished = 1;
	}
    //verify receive_msg == send_msg
    while(1) {
        uart1_put_string("proc2: \r\n");
        release_processor();
    }
}

/*
reciver message from proc1, doesn't block
set proc2 prority to LOW
proc4 get run
send msg to proc4


*/
void proc_p2_3(void) {
	MSG_BUF *msg = NULL;
	char send_msg[6] = "Blah3";
	char receive_msg[6];
	int sender_id;
	
	msg = (MSG_BUF*)receive_message(&sender_id);
	#ifdef DEBUG_0
		printf("proc3 got message from proc %d : %s",(sender_id), msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg, msg->mtext, 6);
	
	set_process_priority(PID_P2, LOW);
	
	strncpy(msg->mtext, send_msg, strlen(send_msg)+1);
	#ifdef DEBUG_0
		printf("proc3 send message to itself: %s", msg->mtext);
	#endif /* DEBUG_0 */
	send_message(PID_P4, msg);
	
	set_process_priority(PID_P3, LOWEST);
	
	if(strcmp(receive_msg,"Blah1") == 0 && sender_id == PID_P1){
			passed_test[2] = 1;
	}
	
	while(1) {
		uart1_put_string("proc3: \r\n");
		release_processor();
	}
	
}

/*
Proc4:
receive message, message comes from proc3
block
proc3 get run
send message to proc1
switch to proc

*/
void proc_p2_4(void) {
	MSG_BUF *msg = NULL;
	char send_msg[6] = "Blah4";
	char receive_msg[6];
	int sender_id;
	
	msg = receive_message(&sender_id);
	#ifdef DEBUG_0
		printf("proc4 got message from proc %d : %s",(sender_id), msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg, msg->mtext, strlen(msg->mtext)+1);
	
	strncpy(msg->mtext, send_msg, strlen(send_msg)+1);
	#ifdef DEBUG_0
		printf("proc4 send message to proc %d : %s" , PID_P3, msg->mtext);
	#endif /* DEBUG_0 */
	send_message(PID_P1, msg);
	
	
	set_process_priority(PID_P4, LOWEST);
	
	if(strcmp(receive_msg,"Blah3") == 0 && sender_id == PID_P3){
			passed_test[3] = 1;
	}
	
	while(1) {
		uart1_put_string("proc4: \r\n");
		release_processor();
	}	
}

void proc_p2_5(void) {
	void *p_mem_blk1;
	int ret_val = 20;

	#ifdef DEBUG_0
		printf("Entering proc5, proc5 says hello\r\n");
	#endif /* DEBUG_0 */

	
	p_mem_blk1 = request_memory_block();
	#ifdef DEBUG_0
		printf("proc3: p_mem_blk=0x%x\r\n", p_mem_blk1);
	#endif /* DEBUG_0 */

	ret_val = release_memory_block(p_mem_blk1);
	#ifdef DEBUG_0
		printf("proc3: ret_val=%d\r\n", ret_val);
	#endif /* DEBUG_0 */

	set_process_priority(PID_P5, LOWEST);	
	
	if(ret_val == RTX_OK){
			passed_test[4] = 1;
	}
	
	while(1) {
		uart1_put_string("proc5 says yo \r\n");
		release_processor();
	} 
	

}


void proc_p2_6(void) {
		int i;
		int success = 0;
		int fail = 0;
    while(1){
			uart1_put_string("proc6 says yooo \r\n");
			#ifdef DEBUG_0
			if(isFinished){
				printf("START\r\n");
				for(i=0; i < NUM_TEST_PROCS; i++){
					if(passed_test[i] == 1){
						printf("test %d OK\r\n", i+1);
						success += 1;
					}else{
						printf("test %d FAIL\r\n",i+1);
						fail += 1;
					}
				}
				printf("%d/%d tests OK \r\n",success,NUM_TEST_PROCS);
				printf("%d/%d tests FAIL \r\n",fail,NUM_TEST_PROCS);
				printf("END\r\n");
				isFinished = 0;
			}
			#endif
			release_processor();
		}
    
}
