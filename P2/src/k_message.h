#ifndef K_MSG_H_
#define K_MSG_H_
#include "k_rtx.h"

void message_log_buffer_init(void); // init log buffer time to NOT_SET
void update_send_log_buffer(MSG_T* message);
void update_receive_log_buffer(MSG_T* message);

int k_send_message(int receiving_pid, void *message_envelope);
void* k_receive_message(int *sender_id);
int k_delayed_send(int receiver_pid, void *message_envelope, int delay);
int k_send_message_nonpreempt(U32 receiver_pid, void *message_envelope);

#endif /* ! K_MEM_H_ */
