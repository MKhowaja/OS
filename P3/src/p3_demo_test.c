#include "rtx.h"
#include "uart_polling.h"
#include "p3_demo_test.h"
#include "string.h"
#include <stdlib.h>
#include "printf.h"
#ifdef DEBUG_0

#endif /* DEBUG_0 */
#define TEN_SEC 5000

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
    
    g_test_procs[0].m_pid = PID_CLOCK;
  g_test_procs[0].mpf_start_pc = &clock_proc;
    g_test_procs[0].m_priority = HIGH;
    
    g_test_procs[1].m_pid = PID_SET_PRIO;
    g_test_procs[1].mpf_start_pc = &set_process_proc;
    g_test_procs[1].m_priority   = HIGH;
    
    g_test_procs[2].m_pid = PID_A;  
    g_test_procs[2].mpf_start_pc = &stress_test_proc_a;
    g_test_procs[2].m_priority   = LOW;
    
    g_test_procs[3].m_pid = PID_B;
    g_test_procs[3].mpf_start_pc = &stress_test_proc_b;
    g_test_procs[3].m_priority   = LOW;
    
    g_test_procs[4].m_pid = PID_C;
    g_test_procs[4].mpf_start_pc = &stress_test_proc_c;
    g_test_procs[4].m_priority   = LOW;
    
    uart1_init();
}

void msg_queue_enque(MSG_BUF** msg_queue, MSG_BUF* message){
    MSG_BUF* message_queue_iter = NULL;
    message_queue_iter = *msg_queue;
		message->mp_next = NULL;
    if (message_queue_iter == NULL){
        *msg_queue = message;
    }
    else {
        while (message_queue_iter->mp_next !=NULL){
            message_queue_iter = (MSG_BUF*)message_queue_iter->mp_next;
        }
				if (message_queue_iter == message){
						printf("PANIC 2\r\n");
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


int substr_atoi(char* s, int n) {
	int value  = 0, base  = 1;
	while (n > 0) {
		value += base * (s[--n] - '0');
		base  *= 10;
	}
	return value;
}

void set_time(char* mtext, int track_time) {
	int seconds = track_time % 60;
	int minutes = track_time / 60 % 60;
	int hours = track_time / 60 / 60 % 24;
    // 0 1 2 3 4 5 6 7 8 9 A B
    // % W S _ H H : M M : S S
	
	mtext[0] = hours / 10 + '0';
	mtext[1] = hours % 10 + '0';
	mtext[2] = ':';
	mtext[3] = minutes / 10 + '0';
	mtext[4] = minutes % 10 + '0';
	mtext[5] = ':';
	mtext[6] = seconds / 10 + '0';
	mtext[7] = seconds % 10 + '0';
	mtext[8] = '\n';
	mtext[9] = '\r';
	mtext[10] = '\0';
}

void clock_proc(void){
    MSG_BUF* message;
	MSG_BUF* update_message;
	char cmd [] = "%W";
	int sender_id;
	int track_time = 0;
	int running = 0;
	
	message = (MSG_BUF*)request_memory_block();
    //reg cmd
    message->mtype = KCD_REG;
    strcpy(message->mtext, cmd);
	send_message(PID_KCD, (void*)message);
	
	while(1) {
		message = (MSG_BUF*)receive_message(&sender_id);
		
		if (message != NULL) {
			if (running == 1 && sender_id == PID_CLOCK) { // start the clock
				// send to clock itself and run the clock every one second
				update_message = (MSG_BUF*)request_memory_block();
				update_message->mtype = DEFAULT;
				delayed_send(PID_CLOCK, update_message, 1000);
				
				// send the time to CRT to display
				update_message = (MSG_BUF*)request_memory_block();
				update_message->mtype = CRT_DISPLAY;
				set_time(update_message->mtext, track_time);
				send_message(PID_CRT, update_message);
				
				// increment the time every one second
				track_time++;
			} else { // not currently running or sender is not clock itself
				switch (message->mtext[2]) {
					case 'R': {
						track_time = 0;
						if (running == 0) {
							// send to itself when first it is run, to trigger the clock to start
							update_message = (MSG_BUF*)request_memory_block();
							message->mtype = DEFAULT;
							send_message(PID_CLOCK, (void*)update_message);
                            //start clock
							running = 1;
						}
						
						break;
					}
					case 'S': {
						int hours = substr_atoi(&(message->mtext[4]), 2);
						int minutes = substr_atoi(&(message->mtext[7]), 2);
						int seconds = substr_atoi(&(message->mtext[10]), 2);
						
						track_time = hours * 3600 + minutes * 60 + seconds;
						if (running == 0) {
							// send to itself when first it is run, to trigger the clock to start
							update_message = (MSG_BUF*)request_memory_block();
							message->mtype = DEFAULT;
							send_message(PID_CLOCK, (void*)update_message);
                            //start clock
							running = 1;
						}
						
						break;
					}
					case 'T': {
						running = 0;
					}
				}
			}
			
			release_memory_block(message);
		}
	}
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
    #ifdef SET_PRIORITY_0
        printf("set_proc send a message to kcd");
    #endif
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
            #ifdef SET_PRIORITY_0
                printf("ERROR! Set Process Proc got invalid message\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        if (buf[4] == ' '){
            space_loc = 4;
            if (buf[3] < 48 || buf[3] > 57) { //NaN
                #ifdef SET_PRIORITY_0
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
                #ifdef SET_PRIORITY_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[0] = buf[3];
            if (buf[4] < 48 || buf[4] > 57) { //NaN
                #ifdef SET_PRIORITY_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[1] = buf[4];
            buf_to_int[2] = '\0';
        }
        if (space_loc+2 != strlen(buf)-2){
            #ifdef SET_PRIORITY_0
                printf("ERROR! Set Process Proc got invalid chars\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        pid = atoi(buf_to_int);
        if (pid < 1 || pid > 13){ //should not allow change priority of NULL or I PROCS
            #ifdef SET_PRIORITY_0
                printf("ERROR! Set Process Proc got restricted pid\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        if (buf[space_loc] != ' '){
            #ifdef SET_PRIORITY_0
                printf("ERROR! Set Process Proc got invalid message\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        if (buf[space_loc+1] < 48 || buf[space_loc+1] > 57) { //NaN
            #ifdef SET_PRIORITY_0
                printf("ERROR! Set Process Proc got NaN for Priority\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        buf_to_int[0] = buf[space_loc + 1];
        buf_to_int[1] = '\0';
        priority = atoi(buf_to_int);
        if (priority < 0 || priority > 3){
            #ifdef SET_PRIORITY_0
                printf("ERROR! Set Process Proc got Invalid Priority\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        set_process_priority (pid, priority);
    }
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
    #ifdef DEBUG_0
        printf("a send a message to kcd");
    #endif
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
    char message[12] = "Process C\r\n\0";
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
           if(atoi(msg_p->mtext)%20==0){ //msg_p will be released in uart
               //send message to crt display
               msg_p->mtype = CRT_DISPLAY;
               strncpy(msg_p->mtext, message, strlen(message) + 1); // message is "Process C"
				#ifdef DEBUG_0
					printf("stress process c send a message to crt");
				 #endif
               send_message(PID_CRT,msg_p); //message p gets released after crt sends it to uart

                //hibernate for 10 sec
                //request msg and delayed send for 10 sec
               msg_q = (MSG_BUF *) request_memory_block();
                msg_q->mtype = WAKEUP10;
								strncpy(msg_q->mtext, "delay send C\r\n", strlen("delay send C\r\n")+1);
                delayed_send(PID_C, msg_q, TEN_SEC);
                while(1){
                   msg_p = (MSG_BUF *) receive_message(&sender_id);

                    if(msg_p->mtype == WAKEUP10){
                       //release_memory_block(msg_p);    //release memory block that was requested for delay send;
                        break;
                    }
                    else{
											#ifdef DEBUG_0
												printf("stress process c is enqueue message\r\n");
											#endif
                       msg_queue_enque(&msg_queue, msg_p);
                    }
                }
            }//else {
							//release_memory_block(msg_p); //Need to free msg_p if not going to uart 
						//}
        }//else{
				//#ifndef DEBUG_0
					//printf("PANIC!!!");
				//#endif
        release_memory_block(msg_p); //Need to free msg_p if not going to uart 
				//}
        
       // not releasing p here because uart proc will do it eventually
       release_processor();
    }


}


