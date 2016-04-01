#include "k_message.h"
#include "k_process.h"
#include "string.h"
#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */
#include "timer.h"

#define NOT_SET 0xFFFFFFFF
static LOG_MSG_BUF send_log_buffer[NUM_MSG_BUFFERED];
static LOG_MSG_BUF receive_log_buffer[NUM_MSG_BUFFERED];
static int send_log_buffer_index = 0;
static int receive_log_buffer_index = 0;

void message_log_buffer_init(void){
	int i;
	for (i = 0; i < NUM_MSG_BUFFERED; i++){
		send_log_buffer[i].timestamp = NOT_SET;
		receive_log_buffer[i].timestamp = NOT_SET;
	}
}

void update_send_log_buffer(MSG_BUF* message){
	send_log_buffer[send_log_buffer_index].sender_pid = message->sender_pid;
	send_log_buffer[send_log_buffer_index].receiver_pid = message->receiver_pid;
	send_log_buffer[send_log_buffer_index].mtype = message->mtype;
	// copy the first 16 bytes
	strncpy(send_log_buffer[send_log_buffer_index].mtext, message->mtext, 16);
	//make sure it is NULL terminated
	send_log_buffer[send_log_buffer_index].mtext[16] = '\0';
	//send_log_buffer[send_log_buffer_index].timestamp = get_timer_count();
	// update the index of the next override
	send_log_buffer_index = (send_log_buffer_index + 1) % NUM_MSG_BUFFERED;
}

void update_receive_log_buffer(MSG_BUF* message){
	receive_log_buffer[receive_log_buffer_index].sender_pid = message->sender_pid;
	receive_log_buffer[receive_log_buffer_index].receiver_pid = message->receiver_pid;
	receive_log_buffer[receive_log_buffer_index].mtype = message->mtype;
	// // copy the first 16 bytes
	strncpy(receive_log_buffer[receive_log_buffer_index].mtext, message->mtext, 16);
	// make sure it is NULL terminated
	receive_log_buffer[receive_log_buffer_index].mtext[16] = '\0';
	//receive_log_buffer[receive_log_buffer_index].timestamp = get_timer_count();
	// update the index of the next override
	receive_log_buffer_index = (receive_log_buffer_index + 1) % NUM_MSG_BUFFERED;
}

void print_send_log_buffer(void){
	int i;
	for (i = 0; i < NUM_MSG_BUFFERED; i++){
		if (send_log_buffer[i].timestamp != NOT_SET){
				#ifdef DEBUG_0 
				printf("Sender pid: %d, Receiver pid: %d, Message type: %d, First 16 bytes: %s, timestamp: %d \r\n", 
				send_log_buffer[i].sender_pid,
				send_log_buffer[i].receiver_pid,
				send_log_buffer[i].mtype, 
				send_log_buffer[i].mtext,
				send_log_buffer[i].timestamp
			);
			#endif
		}
	}
}

void printMessageQ(PCB* receiver){
		MSG_BUF* message_queue_iter = NULL;
		int i = 1;
		if(receiver == NULL){
			#ifdef DEBUG_0
				printf("panic");
			#endif
		}
		message_queue_iter = receiver->msg_queue;
	#ifdef DEBUG_0
		while(message_queue_iter != NULL){
			if(message_queue_iter == (MSG_BUF*)message_queue_iter->mp_next){
						#ifdef DEBUG_0
							printf("This message address is 0x%x, but its next is itself, 0x%x,  hahahaha it's funny, stop joking!\r\n", message_queue_iter,message_queue_iter->mp_next);
						#endif
			}
			printf("process %d's %dth message is 0x%x, with text %s\r\n", receiver->m_pid, i, message_queue_iter, message_queue_iter->mtext);
			message_queue_iter = (MSG_BUF*)message_queue_iter->mp_next;
			i++;
		}
	#endif
}

void message_enque(PCB* receiver, MSG_BUF* message){
	MSG_BUF* message_queue_iter = NULL;
	message_queue_iter = receiver->msg_queue;
	
	printMessageQ(receiver);
	#ifdef DEBUG_0
		printf("getting message 0x%x, message->next is 0x%x\r\n", message, message->mp_next);
	#endif
	
	if (message_queue_iter == NULL){
			receiver->msg_queue = message;
	}
	else {
			while (message_queue_iter->mp_next !=NULL){
				
					message_queue_iter = (MSG_BUF*)message_queue_iter->mp_next;
				if(message_queue_iter == (MSG_BUF*)message_queue_iter->mp_next){
						#ifdef DEBUG_0
							printf("This message address is 0x%x, but its next is itself, 0x%x,  hahahaha it's funny, stop joking!\r\n", message_queue_iter,message_queue_iter->mp_next);
						#endif
				}
			}
			message_queue_iter->mp_next = message;
	}
	//update_send_log_buffer(message);
}

MSG_BUF* message_deque (PCB* receiver){
	MSG_BUF* message = NULL;
	//printMessageQ(receiver);
	if (receiver->msg_queue == NULL){
		return NULL;
	}
	else {
		//printMessageQ(receiver);
		message = receiver->msg_queue;
		receiver->msg_queue = (MSG_BUF*)receiver->msg_queue->mp_next;
		message->mp_next = NULL;
	}
	
	//printMessageQ(receiver);
	#ifdef DEBUG_0
		printf("removing message 0x%x, message->next is 0x%x\r\n", message, message->mp_next);
	#endif
	//update_receive_log_buffer(message);
	return message;
	
}


void print_receive_log_buffer(void){
	int i;
	for (i = 0; i < NUM_MSG_BUFFERED; i++){
		if (receive_log_buffer[i].timestamp != NOT_SET){
				#ifdef DEBUG_0 
					printf("Sender pid: %d, Receiver pid: %d, Message type: %d, First 16 bytes: %s, timestamp: %d \r\n", 
				receive_log_buffer[i].sender_pid,
				receive_log_buffer[i].receiver_pid,
				receive_log_buffer[i].mtype, 
				receive_log_buffer[i].mtext,
				receive_log_buffer[i].timestamp
			);
			#endif
		}
	}
}

int k_send_message(int receiver_pid, void *message_envelope)
{
	PCB * receiver_pcb = NULL;
	MSG_BUF* message = NULL;
	PCB * current_process = NULL;
	
	#ifdef DEBUG_0
        printf("Entering k_send_message\r\n");
  #endif /* DEBUG_0 */
	
	if(receiver_pid == 0){
		#ifdef DEBUG_0
				printf("Why is pid == 0 here");
		#endif
	}
	
	__disable_irq();
	current_process = k_get_current_process();
	if (message_envelope == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message = (MSG_BUF*)message_envelope;

	receiver_pcb = k_get_pcb_from_id ((U32) receiver_pid);
	
	if(receiver_pcb == NULL || receiver_pcb->m_pid == 0){
			#ifdef DEBUG_0
				printf("WTF, ARE YOU KIDDING ME!");
			#endif
	}
	
	if (receiver_pcb == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	message->mp_next = NULL;
	
	message_enque(receiver_pcb, message);
	
	if(handle_msg_blocked_process_ready(receiver_pcb)){
		__enable_irq();
		k_release_processor();
		__disable_irq();
	}
	
	__enable_irq();
	return RTX_OK;
}

int k_send_message_nonpreempt(U32 receiver_pid, void *message_envelope)
{
	PCB * receiver_pcb = NULL;
	MSG_BUF* message = NULL;
	//PCB * current_process;

	message = (MSG_BUF*)message_envelope;
	//current_process = k_get_current_process();

	receiver_pcb = k_get_pcb_from_id ((U32) receiver_pid);
	if (receiver_pcb == NULL){
		return RTX_ERR;
	}
	
	// comment out sender_pid because delay send may have a different current running process 
	// this function is also used in uart_irq, not sure if this hack works, keep that in mind
	//message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	message->mp_next = NULL;
	
	message_enque(receiver_pcb, message);
	
	return handle_msg_blocked_process_ready(receiver_pcb);
}

// Note: sender_id is an output parameter
void* k_receive_message(int *sender_id)
{
	MSG_BUF* message = NULL;
	PCB * current_process = NULL;
	#ifdef DEBUG_0
        printf("Entering k_receive_message\r\n");
  #endif /* DEBUG_0 */
	__disable_irq();

	current_process = k_get_current_process();
	while (current_process->msg_queue == NULL){
		//Setting state to msg blocked will cause scheduler to put process in blocked queue
		current_process->m_state = MSG_BLOCKED;
		__enable_irq();
		k_release_processor();
		__disable_irq();
	}
	//msg queue not empty 
	//message = (MSG_BUF*) linkedList_pop_front(&(current_process->m_msg_queue));
	message = message_deque(current_process);

	if (message == NULL){
		#ifdef DEBUG_0
			printf("PANIC RECEIVE MESSAGE WAS NULL\r\n");
		#endif /* DEBUG_0 */
	}else if (message->sender_pid != NULL){
		*sender_id = message->sender_pid;
	}
	__enable_irq();
	return (void*) message;
}

//SHOULD ONLY BE USED BY UART_I_PROC
void* k_receive_message_nonblocking(int *sender_id){
	MSG_BUF* message = NULL;
	PCB * receiver_pcb = NULL;
	#ifdef DEBUG_0
        printf("Entering k_receive_message non blocking\r\n");
  	#endif /* DEBUG_0 */
	receiver_pcb = k_get_pcb_from_id(PID_UART_IPROC);
	if (receiver_pcb == NULL){
		#ifdef DEBUG_0
			printf("PANIC UART PCB WAS NULL IN RECEIVE MESSAGE NONBLOCKING\r\n");
		#endif /* DEBUG_0 */
		return NULL;
	}
	else if (receiver_pcb->msg_queue == NULL){
		#ifdef DEBUG_0
			printf("No message for UART to receive\r\n");
		#endif /* DEBUG_0 */
		return NULL;
	}
	message = message_deque(receiver_pcb);
	if (message == NULL){
		#ifdef DEBUG_0
			printf("PANIC UART RECEIVE MESSAGE WAS NULL\r\n");
		#endif /* DEBUG_0 */
	}else if (message->sender_pid != NULL){
		*sender_id = message->sender_pid;
	}
	return (void*) message;
}

int k_delayed_send(int receiver_pid, void *message_envelope, int delay){
	MSG_BUF* message = NULL;
	PCB * current_process = NULL;
	PCB * pcb = NULL;
	#ifdef DEBUG_0
        printf("Entering k_delayed_send\r\n");
  #endif /* DEBUG_0 */
	
	__disable_irq();
	
	
	if (message_envelope == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	
	pcb = k_get_pcb_from_id ((U32) receiver_pid);
	if (pcb == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message = (MSG_BUF*)message_envelope;
	current_process = k_get_current_process();

	message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	message->msg_delay = delay;

	expire_list_enqueue(message);

	__enable_irq();
	return RTX_OK;
}

