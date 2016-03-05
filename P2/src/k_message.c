#include "k_message.h"
#include "k_process.h"
#include "string.h"
#ifdef DEBUG_0
#include "printf.h"
#include "timer.h"
#endif /* ! DEBUG_0 */

#define NOT_SET 0xFFFFFFFF
static LOG_MSG_T send_log_buffer[NUM_MSG_BUFFERED];
static LOG_MSG_T receive_log_buffer[NUM_MSG_BUFFERED];
static volatile int send_log_buffer_index = 0;
static volatile int receive_log_buffer_index = 0;

void message_log_buffer_init(void){
	int i;
	for (i = 0; i < NUM_MSG_BUFFERED; i++){
		send_log_buffer[i].timestamp = NOT_SET;
		receive_log_buffer[i].timestamp = NOT_SET;
	}
}

void update_send_log_buffer(MSG_T* message){
	send_log_buffer[send_log_buffer_index].sender_pid = message->sender_pid;
	send_log_buffer[send_log_buffer_index].receiver_pid = message->receiver_pid;
	send_log_buffer[send_log_buffer_index].msg_type = message->msg_type;
	// copy the first 16 bytes
	strncpy(send_log_buffer[send_log_buffer_index].mText, message->mText, 16);
	// make sure it is NULL terminated
	send_log_buffer[send_log_buffer_index].mText[16] = '\0';
	send_log_buffer[send_log_buffer_index].timestamp = get_timer_count();
	// update the index of the next override
	send_log_buffer_index = (send_log_buffer_index + 1) % NUM_MSG_BUFFERED;
}

void update_receive_log_buffer(MSG_T* message){
	receive_log_buffer[receive_log_buffer_index].sender_pid = message->sender_pid;
	receive_log_buffer[receive_log_buffer_index].receiver_pid = message->receiver_pid;
	receive_log_buffer[receive_log_buffer_index].msg_type = message->msg_type;
	// copy the first 16 bytes
	strncpy(receive_log_buffer[receive_log_buffer_index].mText, message->mText, 16);
	// make sure it is NULL terminated
	receive_log_buffer[receive_log_buffer_index].mText[16] = '\0';
	receive_log_buffer[receive_log_buffer_index].timestamp = get_timer_count();
	// update the index of the next override
	receive_log_buffer_index = (receive_log_buffer_index + 1) % NUM_MSG_BUFFERED;
}

void print_send_log_buffer(void){
	int i;
	for (i = 0; i < NUM_MSG_BUFFERED; i++){
		if (send_log_buffer[i].timestamp != NOT_SET){
			printf("Sender pid: %d, Receiver pid: %d, Message type: %d, First 16 bytes: %s, timestamp: %d \r\n", 
				send_log_buffer[i].sender_pid,
				send_log_buffer[i].receiver_pid,
				send_log_buffer[i].msg_type, 
				send_log_buffer[i].mText,
				send_log_buffer[i].timestamp
			);
		}
	}
}


void print_receive_log_buffer(void){
	int i;
	for (i = 0; i < NUM_MSG_BUFFERED; i++){
		if (receive_log_buffer[i].timestamp != NOT_SET){
			printf("Sender pid: %d, Receiver pid: %d, Message type: %d, First 16 bytes: %s, timestamp: %d \r\n", 
				receive_log_buffer[i].sender_pid,
				receive_log_buffer[i].receiver_pid,
				receive_log_buffer[i].msg_type, 
				receive_log_buffer[i].mText,
				receive_log_buffer[i].timestamp
			);
		}
	}
}

int k_send_message(int receiver_pid, void *message_envelope)
{
	int successCode;
	PCB * receiver_pcb;
	MSG_T* message;
	PCB * current_process;
		
	__disable_irq();
	current_process = k_get_current_process();
	if (message_envelope == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message = (MSG_T*)message_envelope;
	if (receiver_pid < NUM_TEST_PROCS){ // sending to kernel proc not allowed
		__enable_irq();
		return RTX_ERR;
	}
	receiver_pcb = k_get_pcb_from_id ((U32) receiver_pid);
	if (receiver_pcb == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	
	successCode = linkedList_push_back(&(receiver_pcb->m_msg_queue), (void *) message);
	if (successCode == 1) {
		 //error inserting to list because list was null
		update_send_log_buffer(message);
		__enable_irq();
		return RTX_ERR;
	}
	
	// if (pcb->m_state == MSG_BLOCKED){
	// 	pcb->m_state = RDY;
	// 	ready_enqueue (pcb);
	// }
	// check if there is process blocked on msg

	//if(handle_blocked_process_ready(MSG_BLOCKED)){
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
	PCB * pcb;
	MSG_T* message;
	PCB * current_process;

	message = (MSG_T*)message_envelope;
	current_process = k_get_current_process();

	pcb = k_get_pcb_from_id ((U32) receiver_pid);
	if (pcb == NULL){
		return RTX_ERR;
	}
	
	message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	
	linkedList_push_back(&(pcb->m_msg_queue), (void *) message);
	
	// if (pcb->m_state == MSG_BLOCKED){
	// 	pcb->m_state = RDY;
	// 	ready_enqueue (pcb);
	// }
	return handle_blocked_process_ready(MSG_BLOCKED);

	// if(handle_blocked_process_ready(MSG_BLOCKED)){
	// 	__enable_irq();
	// 	k_release_processor();
	// 	__disable_irq();
	// }
	
	// __enable_irq();
	// return RTX_OK;
}

// Note: sender_id is an output parameter
void* k_receive_message(int *sender_id)
{
	MSG_T* message;
	PCB * current_process;
	__disable_irq();
	current_process = k_get_current_process();
	while (current_process->m_msg_queue.length == 0){
		//TODO VERA
		current_process->m_state = MSG_BLOCKED;
		__enable_irq();
		k_release_processor();
		__disable_irq();
	}
	//msg queue not empty 
	message = (MSG_T*) linkedList_pop_front(&(current_process->m_msg_queue));
	if (message->sender_pid != NULL){
		*sender_id = message->sender_pid;
	}
	__enable_irq();
	return (void*) message;
}

int k_delayed_send(int receiver_pid, void *message_envelope, int delay){
	MSG_T* message;
	PCB * current_process;
	PCB * pcb;
	__disable_irq();
	if (message_envelope == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	if (receiver_pid < NUM_TEST_PROCS){ // sending to kernel proc not allowed
		__enable_irq();
		return RTX_ERR;
	}
	pcb = k_get_pcb_from_id ((U32) receiver_pid);
	if (pcb == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message = (MSG_T*)message_envelope;
	current_process = k_get_current_process();

	message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	message->msg_delay = delay;

	expire_list_queue(message);

	__enable_irq();
	return RTX_OK;
}

