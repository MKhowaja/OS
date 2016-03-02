#include "k_message.h"

#ifdef DEBUG_0
#include "printf.h"
#include "k_process.h"
#endif /* ! DEBUG_0 */

int k_send_message(U32 receiver_pid, void *message_envelope)
{
	int successCode;
	PCB * pcb;
	MSG_T* message;
	PCB * current_process;
		
	__disable_irq();
	message = (MSG_T*)message_envelope;
	current_process = k_get_current_process();
	if (message_envelope == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	if (receiver_pid < 1 || receiver_pid > NUM_TEST_PROCS){
		__enable_irq();
		return RTX_ERR;
	}
	pcb = k_get_pcb_from_id (receiver_pid);
	if (pcb == NULL){
		__enable_irq();
		return RTX_ERR;
	}
	message->sender_pid = current_process->m_pid;
	message->receiver_pid = receiver_pid;
	
	successCode = linkedList_push_back(&(pcb->m_msg_queue), (void *) message);
	if (successCode == 1) {
		 //error inserting to list because list was null
		__enable_irq();
		return RTX_ERR;
	}
	
	if (pcb->m_state == MSG_BLOCKED){
		pcb->m_state = RDY;
		ready_enqueue (pcb);
	}
	
	__enable_irq();
	return RTX_OK;
}

// Note: sender_id is an output parameter
void* k_receive_message(int *sender_id)
{
	MSG_T* message;
	PCB * current_process;
	__disable_irq();
	current_process = k_get_current_process();
	while (current_process->m_msg_queue.length == 0){
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
