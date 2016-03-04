#ifndef K_MSG_H_
#define K_MSG_H_
#include "k_rtx.h"


int k_send_message(U32 receiving_pid, void *message_envelope);
void* k_receive_message(int *sender_id);

#endif /* ! K_MEM_H_ */
