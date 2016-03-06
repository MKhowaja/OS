#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "string.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
int passed_test[NUM_TEST_PROCS];

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		//g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=0x200;
		passed_test[i] = 0;
	}
	passed_test[5] = 1;
  
	
	//g_test_procs[0].mpf_start_pc = &proc1;
	//g_test_procs[0].m_priority   = HIGH;
	
	//g_test_procs[1].mpf_start_pc = &proc2;
	//g_test_procs[1].m_priority   = HIGH;
	
	//g_test_procs[2].mpf_start_pc = &proc3;
	///g_test_procs[2].m_priority   = MEDIUM;
	
	//g_test_procs[3].mpf_start_pc = &proc4;
	//g_test_procs[3].m_priority   = MEDIUM;
	
	//g_test_procs[4].mpf_start_pc = &proc5;
	//g_test_procs[4].m_priority   = MEIDUM;
	
	//g_test_procs[5].mpf_start_pc = &proc6;
	//g_test_procs[5].m_priority   = LOWEST;
	
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
	
	uart1_init();
}

/*
proc 1: high
request block
output
request block
output
set proc 2 priority to high
output
release 1st block
output
release 2nd block
set priority to lowest
while
	output
	release processor
*/
void proc1(void)
{
	int ret_val = 20;
	void *p_mem_blk1;
	void *p_mem_blk2;
	#ifdef DEBUG_0
			printf("Entering proc1\r\n");
	#endif /* DEBUG_0 */
	p_mem_blk1 = request_memory_block();
	#ifdef DEBUG_0
			printf("proc1: p_mem_blk1=0x%x\r\n", p_mem_blk1);
	#endif /* DEBUG_0 */
	p_mem_blk2 = request_memory_block();
	#ifdef DEBUG_0
			printf("proc1: p_mem_blk2=0x%x\r\n", p_mem_blk2);
	#endif /* DEBUG_0 */
	set_process_priority(PID_P2, HIGH);
	#ifdef DEBUG_0
			printf("proc1: set proc2 priority to high\r\n");
	#endif /* DEBUG_0 */
	set_process_priority(PID_P1, MEDIUM);
	#ifdef DEBUG_0
			printf("proc1: set priority of proc1 to medium");
	#endif /* DEBUG_0 */
	ret_val = release_memory_block(p_mem_blk1);
	#ifdef DEBUG_0
			printf("proc1: released block 1 ret_val=%d\r\n", ret_val);
	#endif /* DEBUG_0 */
	ret_val = release_memory_block(p_mem_blk2);
	#ifdef DEBUG_0
			printf("proc1: released block 2 ret_val=%d\n", ret_val);
	#endif /* DEBUG_0 */
	set_process_priority(PID_P1, LOWEST);
	#ifdef DEBUG_0
			printf("proc1: set priority of proc1 to lowest");
	#endif /* DEBUG_0 */
	while(1) {
		uart1_put_string("proc1: \r\n");
		release_processor();
	}
}

/**
proc 2: medium
request 1st block
output
request 2nd block
output
release 1st block
output
release 2nd block
output
set priority to lowest
while
	output
	release processor
 */
void proc2(void)
{
	int ret_val = 20;
	void *p_mem_blk1;
	void *p_mem_blk2;
	
	#ifdef DEBUG_0
			printf("Entering proc2\r\n");
	#endif /* DEBUG_0 */
	p_mem_blk1 = request_memory_block();
	#ifdef DEBUG_0
			printf("proc2: p_mem_blk1=0x%x\r\n", p_mem_blk1);
	#endif /* DEBUG_0 */
	p_mem_blk2 = request_memory_block();
	#ifdef DEBUG_0
			printf("proc2: p_mem_blk2=0x%x\r\n", p_mem_blk2);
	#endif /* DEBUG_0 */
	ret_val = release_memory_block(p_mem_blk1);
	#ifdef DEBUG_0
			printf("proc2: released block 1 ret_val=%d\r\n", ret_val);
	#endif /* DEBUG_0 */
	ret_val = release_memory_block(p_mem_blk2);
	#ifdef DEBUG_0
			printf("proc2: released block 2 ret_val=%d\n", ret_val);
	#endif /* DEBUG_0 */
	set_process_priority(PID_P2, LOWEST);
	#ifdef DEBUG_0
			printf("proc2: set priority of proc2 to lowest");
	#endif /* DEBUG_0 */
	while ( 1) {
		uart1_put_string("proc2: \r\n");
		release_processor();
	}
}

void proc3(void)
{
	void *p_mem_blk1;
	void *p_mem_blk2;
	int ret_val = 20;

	#ifdef DEBUG_0
		printf("Entering proc3, proc3 says hello\r\n");
	#endif /* DEBUG_0 */

	p_mem_blk1 = request_memory_block();
	#ifdef DEBUG_0
		printf("proc3: p_mem_blk=0x%x\r\n", p_mem_blk1);
	#endif /* DEBUG_0 */

	p_mem_blk2 = request_memory_block();
	#ifdef DEBUG_0
		printf("proc3: p_mem_blk=0x%x\r\n", p_mem_blk2);
	#endif /* DEBUG_0 */

	ret_val = release_memory_block(p_mem_blk1);
	ret_val = release_memory_block(p_mem_blk1);
	#ifdef DEBUG_0
		printf("proc3: ret_val=%d\r\n", ret_val);
	#endif /* DEBUG_0 */

	ret_val = release_memory_block(p_mem_blk2);
	#ifdef DEBUG_0
		printf("proc3: ret_val=%d\r\n", ret_val);
	#endif /* DEBUG_0 */

	set_process_priority(PID_P3, LOWEST);	
	while(1) {
		uart1_put_string("proc3 says yo \r\n");
		release_processor();
	}
}

void proc4(void)
{
	#ifdef DEBUG_0
	printf("entering proc4\r\n");
	#endif /* DEBUG_0 */
	set_process_priority(PID_P4, LOWEST);
	#ifdef DEBUG_0
	printf("set proc4 priority to LOWEST\r\n");
	#endif /* DEBUG_0 */
	while(1){
		#ifdef DEBUG_0
		printf("while loop in proc4\r\n");
		#endif /* DEBUG_0 */
		release_processor();
	}
}

void proc5()
{
	#ifdef DEBUG_0
		printf("Entering proc5\r\n");
	#endif
	set_process_priority(PID_P5, LOWEST);
	#ifdef DEBUG_0
	printf("set proc5 priority to LOWEST");
	#endif /* DEBUG_0 */
	while(1){
		#ifdef DEBUG_0
			printf("PORC5: Release processor\r\n");
		#endif
		release_processor();
	}
}

void proc6(void)
{
	#ifdef DEBUG_0
	printf("Entering proc6 \r\n");
	#endif /* DEBUG_0 */
	set_process_priority(PID_P6, LOWEST);
	#ifdef DEBUG_0
	printf("set proc6 priority to LOWEST\r\n");
	#endif /* DEBUG_0 */
	while(1){
		#ifdef DEBUG_0
		printf("while loop in proc6\r\n");
		#endif /* DEBUG_0 */
		release_processor();
	}
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
	char* send_msg = "Blah1";
	char* receive_msg;
	int* sender_id;
	
	msg = (MSG_BUF *) request_memory_block();
	strncpy(msg->mtext, send_msg, strlen(send_msg)+1);
	msg->mtype = DEFAULT;
	
	#ifdef DEBUG_0
		printf("proc2 send message to proc %d : %s" , PID_P3, msg->mtext);
	#endif /* DEBUG_0 */
	send_message(PID_P3,msg);
	
	msg = (MSG_BUF*) receive_message(sender_id);
	#ifdef DEBUG_0
		printf("proc2 got message from proc %d : %s",(*sender_id), msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg, msg->mtext, strlen(msg->mtext)+1);
	
	release_memory_block(msg);
	msg = NULL;
	set_process_priority(PID_P1, LOWEST);
	
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
	char* send_msg = "Blah2";
	char* receive_msg;
	
	msg = (MSG_BUF *) request_memory_block();
	strncpy(msg->mtext,send_msg,strlen(send_msg)+1);
	msg->mtype = DEFAULT;
	
	delayed_send(PID_P2,msg,1000);
	#ifdef DEBUG_0
		printf("proc2 send message to itself: %s", msg->mtext);
	#endif /* DEBUG_0 */
	
	msg = receive_message(NULL);
	#ifdef DEBUG_0
		printf("proc2 got message from itself: %s", msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg,msg->mtext,strlen(msg->mtext)+1);
	release_memory_block(msg);
	
	set_process_priority(PID_P2, LOWEST);
	
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
	char* send_msg = "Blah3";
	char* receive_msg;
	int* sender_id;
	
	msg = (MSG_BUF*)receive_message(sender_id);
	#ifdef DEBUG_0
		printf("proc3 got message from proc %d : %s",(*sender_id), msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg, msg->mtext, 6);
	
	set_process_priority(PID_P2, LOW);
	
	strncpy(msg->mtext, send_msg, strlen(send_msg)+1);
	#ifdef DEBUG_0
		printf("proc3 send message to itself: %s", msg->mtext);
	#endif /* DEBUG_0 */
	send_message(PID_P4, msg);
	
	set_process_priority(PID_P3, LOWEST);
	
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
	char* send_msg = "Blah4";
	char* receive_msg;
	int* sender_id;
	
	msg = receive_message(sender_id);
	#ifdef DEBUG_0
		printf("proc4 got message from proc %d : %s",(*sender_id), msg->mtext);
	#endif /* DEBUG_0 */
	strncpy(receive_msg, msg->mtext, strlen(msg->mtext)+1);
	
	strncpy(msg->mtext, send_msg, strlen(send_msg)+1);
	#ifdef DEBUG_0
		printf("proc4 send message to proc %d : %s" , PID_P3, msg->mtext);
	#endif /* DEBUG_0 */
	send_message(PID_P1, msg);
	
	
	set_process_priority(PID_P4, LOWEST);
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
	while(1) {
		uart1_put_string("proc5 says yo \r\n");
		release_processor();
	} 
	
}


void proc_p2_6(void) {
	
	
}


void clock_proc(void){
	MSG_BUF* read_msg;
	MSG_BUF* msg;
	int sender_id;
	int i;
	char* cmd = "%w";

	int buf_size;
	char read_buf[100];
	buf_size = 100;


	msg = (MSG_BUF*)request_memory_block();

	//reg cmd
	msg->mtype = KCD_REG;
	strncpy(msg->mtext, cmd, strlen(cmd)+1);
	send_message(PID_KCD, msg);

	while(1){
		//clear buf
		for(i = 0; i < buf_size; i++){
			read_buf[i] = '\0';
		}
		//read msg
		read_msg = receive_message(&sender_id);
		strncpy(read_buf, read_msg->mtext, buf_size);

		if( strlen(read_buf) < 3 || read_buf[0] != cmd[0] ||  read_buf[1] != cmd[1]){
			//do nothing, next
			continue;
		}
		
		switch(read_buf[2]){
			case 'R': {
				
				
			}
			
			case 'W': {
				
				
			}
			
			case 'T': {
				
				
			}
			
			
		}


	}
}
