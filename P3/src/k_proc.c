/**
* @file:   kernel_proc.c
* @brief:  Two user processes: proc1 and proc2
* @author: Yiqing Huang
* @date:   2014/02/28
* NOTE: Each process is in an infinite loop. Processes never terminate.
*/

#include "rtx.h"
#include "uart_polling.h"
#include "k_proc.h"
#include "timer.h"
#include "k_message.h"
#include "k_memory.h"
#include "list.h"
#include "string.h"
#include "uart.h"
#include <LPC17xx.h>

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT k_test_procs[NUM_KERNEL_PROCS];

//cmd dictionary
typedef struct cmd_dict_node{
    uint32_t pid;
    char cmd[10];
} CMD_DICT_NODE;

static CMD_DICT_NODE command_buffer [NUM_COMMANDS];
static int command_buffer_index = 0;

char msg_data [200]; //arbitrarily chose size 200

void set_kernel_procs() {
    int i;
    for (i = 0; i < 200; i++) {
        msg_data[i] = '\0';
    }
    //TODO REMOVE I PROCESSES FROM SCHEDULER
    k_test_procs[0].m_pid = PID_NULL;
    k_test_procs[0].m_stack_size = 0X100;
    k_test_procs[0].mpf_start_pc = &nullProc;
    k_test_procs[0].m_priority = 4;//LOWEST;

    k_test_procs[0].mpf_start_pc = &nullProc;
    //add timer i process to k_test_procs

    k_test_procs[1].m_pid = PID_TIMER_IPROC;
    k_test_procs[1].m_stack_size = 0X100;
    k_test_procs[1].mpf_start_pc = &timer_i_process;
    k_test_procs[1].m_priority = 4;

    k_test_procs[2].m_pid = PID_UART_IPROC;
    k_test_procs[2].m_stack_size = 0X100;
    k_test_procs[2].mpf_start_pc = &uart_i_process;
    k_test_procs[2].m_priority = 4;

    //i/o process has highest priority since you need response quick
    k_test_procs[3].m_pid = PID_KCD;
    k_test_procs[3].m_stack_size = 0X100;
    k_test_procs[3].mpf_start_pc = &kcd;
    k_test_procs[3].m_priority = HIGH;

    k_test_procs[4].m_pid = PID_CRT;
    k_test_procs[4].m_stack_size = 0X100;
    k_test_procs[4].mpf_start_pc = &crt;
    k_test_procs[4].m_priority = HIGH;

}

void nullProc (void){
    int ret_val;
		#ifdef DEBUG_0 
			//printf("Null Process Start\n");
		#endif
    while (1){
			#ifdef DEBUG_0 
        //printf("while loop start");
			#endif
        ret_val = release_processor();
        #ifdef DEBUG_0
					//printf("nullProc: ret_val=%d\r\n", ret_val);
        #endif /* DEBUG_0 */
    }
}

//response to crt 
void crt(void){
    MSG_BUF* msg;
		LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*)LPC_UART0;
		int sender_id;

    while(1){
        msg = receive_message(&sender_id); //if current process message queue empty, calls release processor
        if(msg->mtype == CRT_DISPLAY){
        //send msg to uart_i_process
        //we need id of uart_i_process
            //set_g_buffer(msg->mtext);
        if(PID_UART_IPROC == 0){
            #ifdef DEBUG_0
                printf("WTF, why do we wanna send it to null proc, are you serious\n");
            #endif
        }    
        #ifdef DEBUG_0
            printf("crt send a message to uart");
        #endif
				
        send_message(PID_UART_IPROC, msg);
				pUart->IER = pUart->IER | IER_THRE;
        }
        else{
            //do nothing except releasing msg block	
            //release_memory_block(msg);
						#ifdef DEBUG_0
                printf("PANIC\r\n");
            #endif
        }
    }
}

//command reg
//input
void kcd(void){
    int i;
		int j;
		int k;
    //cmd_dict_node *cmd_node;
    int sender_id;
    MSG_BUF* msg;
    char* itr;
    int msg_len;
    char buffer[10];
	
    for (i = 0; i < 10; i++) { //null out buffer
        buffer[i] = '\0';
    }

    while (1) {
        msg = receive_message(&sender_id);

        if (msg->mtype == DEFAULT) {
            //send_message(PID_CRT,msg); //Doing this immediately in UART
            i = 0;

            msg_len = strlen(msg->mtext);
            strncpy(msg_data, msg->mtext, msg_len);
            itr = msg_data;
            if (msg_data[0] == '%') {
								#ifdef DEBUG_0 
								printf("debug *itr %s\r\n",itr);
								#endif
                while (*itr != ' ' && *itr != '\r') {
                    if (*itr == '\0') {
                        *itr++;
                    } else {
                        buffer[i++] = *itr++;
                    }
                }
								release_memory_block(msg);
								msg = NULL;
                if (i < 7){ //buffer length
										buffer[i++] = '\r';
										buffer[i++] = '\n';
                    buffer[i++] = '\0';
                    for (i = 0; i < command_buffer_index; i++){
											msg_len = strlen(command_buffer[i].cmd);
											k = 1;
											for(j = 0; j < msg_len; j++){
												if (command_buffer[i].cmd[j] == buffer[j]){
													continue;
												}
												else {
													k = 0;
													break;
												}
											}
                        if (k == 1){
                            msg = (MSG_BUF*) request_memory_block();
                            msg->mtype = DEFAULT;
                            strcpy(msg->mtext, msg_data);
                            if(command_buffer[i].pid == 0){
                                #ifdef DEBUG_0
                                    printf("WTF, why do we wanna send it to null proc, are you serious\r\n");
                                #endif
                            }
                            #ifdef DEBUG_0
                                    printf("kcd send a message to %d",command_buffer[i].pid);
                            #endif
                            send_message(command_buffer[i].pid, msg);
													#ifdef DEBUG_0 
                            printf ("Sent message with command %d\r\n", i);
													#endif
                            break;
                        }
                    }    
                }
                
            }

            for (i = 0; i < 10; i++) {
                buffer[i] = '\0';
            }
            for (i = 0; i < 200; i++) {
                msg_data[i] = '\0';
            }
            continue;
        } else if (msg->mtype == KCD_REG) {
            if (command_buffer_index < NUM_COMMANDS){
							#ifdef DEBUG_0
								printf("kcd is registering command: %s\r\n", msg->mtext);
							#endif
                command_buffer[command_buffer_index].pid = sender_id;
                strcpy(command_buffer[command_buffer_index].cmd, msg->mtext);
                command_buffer_index = (command_buffer_index + 1);
                
            }
            else {
								#ifdef DEBUG_0 
                printf("Command is full\r\n");
								#endif
            }
            release_memory_block(msg);
        }
        else{
            //has to be CRT DISPLAY
            //msg no need to change
            #ifdef DEBUG_0
                 printf("kdc send a message to crt");
            #endif
            send_message(PID_CRT,msg);
        }
    }
}
