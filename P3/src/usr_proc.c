#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "string.h"
#include <stdlib.h>

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */
#define TEN_SEC 10000

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
	g_test_procs[0].m_pid = PID_A;  
	g_test_procs[0].mpf_start_pc = &stress_test_proc_a;
	g_test_procs[0].m_priority   = HIGH;
	
	g_test_procs[1].m_pid = PID_B;
	g_test_procs[1].mpf_start_pc = &stress_test_proc_b;
	g_test_procs[1].m_priority   = HIGH;
	
	g_test_procs[2].m_pid = PID_C;
	g_test_procs[2].mpf_start_pc = &stress_test_proc_c;
	g_test_procs[2].m_priority   = HIGH;
	
	uart1_init();
}



void msg_queue_enque(MSG_BUF** msg_queue, MSG_BUF* message){
	MSG_BUF* message_queue_iter = NULL;
	message_queue_iter = *msg_queue;
	if (message_queue_iter == NULL){
		*msg_queue = message;
	}
	else {
		while (message_queue_iter->mp_next !=NULL){
			message_queue_iter = (MSG_BUF*)message_queue_iter->mp_next;
		}
		message_queue_iter->mp_next = message;
	}
}

MSG_BUF* msg_queue_deque (MSG_BUF** msg_queue){
	MSG_BUF* message = NULL;
	if (*msg_queue == NULL){
		return NULL;
	}
	else {
		message = *msg_queue;
		*msg_queue = (MSG_BUF*)((*msg_queue)->mp_next);
		message->mp_next = NULL;
	}
	return message;
	
}


void stress_test_proc_a(void)
{
	MSG_BUF *msg;
	int sender_id, num;
	//char *cmd = "%Z";
	#ifdef DEBUG_0
			printf("Entering Stress Proc A\r\n");
	#endif
	
	
	//initialize msg_buf that would be send to kcd to register
	msg = (MSG_BUF *) request_memory_block();
	msg->mtype = KCD_REG;
	msg->mtext[0] = '%';
	msg->mtext[1] = 'Z';
	msg->mtext[2] = '\0';

	send_message(PID_KCD, msg);
	#ifdef DEBUG_0
			printf("Stress Proc A Loop 1\r\n");
	#endif
	while(1){
		#ifdef DEBUG_0
			printf("Enter %Z to exit loop, any other input outputs ERROR\r\n");
		#endif
		msg = receive_message(&sender_id);
		if(msg && strlen(msg->mtext) >= 2 && msg->mtext[0] == '%' && msg->mtext[1] == 'Z'){
			release_memory_block(msg);
			break;
		}else{
			release_memory_block(msg);
			#ifdef DEBUG_0
				printf("ERROR\r\n");
			#endif
		}
	}

	num = 0;

	while(1){
		#ifdef DEBUG_0
			printf("Stress Proc A Loop 2\r\n");
		#endif
		msg =(MSG_BUF *) request_memory_block();
		msg->mtype = COUNT_REPORT;
		//TODO: need to check if the msg->mtext can be converted back to int in PROCESS c
		sprintf(msg->mtext, "%d", num);
		send_message(PID_B, msg);
		num++;
		release_processor();
	}
}



void stress_test_proc_b(void)
{
	MSG_BUF *msg;
	int sender_id;
	#ifdef DEBUG_0
			printf("Entering Stress Proc B\r\n");
	#endif
	while(1){
		#ifdef DEBUG_0
			printf("Stress Proc B Loop 1\r\n");
		#endif
		msg = (MSG_BUF *) receive_message(&sender_id);
		send_message(PID_C, msg);
	}
}

void stress_test_proc_c(void)
{
	MSG_BUF *msg_p;
	MSG_BUF *msg_q;
	int sender_id;
	char message[10] = "Process C";
	MSG_BUF* msg_queue = NULL;	
	#ifdef DEBUG_0
			printf("Entering Stress Proc C\r\n");
	#endif

	while(1) {
		if(msg_queue == NULL){ //if msg_queue is empty, receive a msg
			msg_p = (MSG_BUF *) receive_message(&sender_id);
		}else{
			msg_p = msg_queue_deque(&msg_queue);
		}

		if(msg_p->mtype == COUNT_REPORT){
			if(atoi(msg_p->mtext)%20==0){ //msg_p will be released in uart eventually
				//send message to crt display
				msg_p->mtype = CRT_DISPLAY;
				strncpy(msg_p->mtext, message, strlen(message) + 1);
				send_message(PID_CRT,msg_p); //message p gets released after crt sends it to uart

				//hibernate for 10 sec
				//request msg and delayed send for 10 sec
				msg_q = (MSG_BUF *) request_memory_block();
				msg_q->mtype = WAKEUP10;
				delayed_send(PID_C, msg_q, TEN_SEC);
				while(1){
					msg_p = (MSG_BUF *) receive_message(&sender_id);
					if(msg_p->mtype == WAKEUP10){
						break;
					}
					else{
						msg_queue_enque(&msg_queue, msg_p);
					}
				}
			}
            else {
                release_memory_block(msg_p); //Need to free msg_p if not going to uart 
            }
		}
        else {
            release_memory_block(msg_p); //Need to free msg_p if not going to uart 
        }
        
        //not releasing p here because uart proc will do it eventually
		release_processor();
	}


}






