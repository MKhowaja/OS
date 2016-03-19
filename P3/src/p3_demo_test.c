#include "rtx.h"
#include "uart_polling.h"
#include "p3_demo_test.h"
#include "string.h"
#include <stdlib.h>
#include "printf.h"
#ifdef DEBUG_0

#endif /* DEBUG_0 */
#define TEN_SEC 2000

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


void clock_proc(void){
    MSG_BUF* read_msg;//read in 
    MSG_BUF* msg;//send out
    int sender_id;
    int i;

    int track_time;
    int oneSec;

    int hour;
    int minute;
    int sec;
    int running;
    char cmd[3] = "%W";

    char buf_to_int[3];

    int buf_size;
    char buf[100];
    buf_size = 100;
    oneSec = 1000;
    running = 0;
    track_time = 0;//0 sec

    msg = (MSG_BUF*)request_memory_block();

    //reg cmd
    msg->mtype = KCD_REG;
    strncpy(msg->mtext, cmd, strlen(cmd)+1);
    send_message(PID_KCD, msg);

    while(1){
        //clear buf
        for(i = 0; i < buf_size; i++){
            buf[i] = '\0';
        }

        //read msg
        read_msg = receive_message(&sender_id);

        if (sender_id == PID_CLOCK && running == 1) {
					// release the msg that delay_send sent
					release_memory_block(read_msg);

            //send after one sec
            msg = (MSG_BUF*)request_memory_block();
            msg->mtype = DEFAULT;
            delayed_send(PID_CLOCK, msg, oneSec);


            hour = (track_time / 3600) % 24;
            minute = (track_time / 60) % 60;
            sec =  track_time % 60;

            // print time
            msg = (MSG_BUF*)request_memory_block();            
            msg->mtype = CRT_DISPLAY;
            sprintf(buf, "%02d:%02d:%02d\n\r", hour, minute, sec); // 69 x-offset = (80 col width - 11 char for HH:MM:SS)
            strncpy(msg->mtext, buf, strlen(buf) + 1);
            send_message(PID_CRT, msg);// print to crt

            //increment 1000 milisecs
            track_time += 1;
            //in case is over one day?
            track_time = track_time % (60 * 60 * 24);
        }
        else{
            strncpy(buf, read_msg->mtext, buf_size);
            release_memory_block(read_msg);

            if( strlen(buf) < 3 || buf[0] != cmd[0] ||  buf[1] != cmd[1]){
                //do nothing, next
                continue;
            }

            if(buf[2] == 'R'){
                track_time = 0;

                if (running == 0) {
                    //start clock
                    msg = (MSG_BUF*)request_memory_block();
                    msg->mtype = DEFAULT;
                    send_message(PID_CLOCK, msg);
                }
                running = 1;
            }
            else if(buf[2] == 'S'){
                // 0 1 2 3 4 5 6 7 8 9 A B
                // % W S _ H H : M M : S S
                buf_to_int[0] = buf[4];
                buf_to_int[1] = buf[5]; 
                buf_to_int[2] = '\0';
                hour = atoi(buf_to_int);

                buf_to_int[0] = buf[7];
                buf_to_int[1] = buf[8];
                buf_to_int[2] = '\0';

                minute = atoi(buf_to_int);

                buf_to_int[0] = buf[10];
                buf_to_int[1] = buf[11];
                buf_to_int[2] = '\0';
                sec = atoi(buf_to_int);

                track_time = hour * 3600 + minute * 60 + sec;

                if (running == 0) {
                    //start clock
                    msg = (MSG_BUF*)request_memory_block();
                    msg->mtype = DEFAULT;
                    send_message(PID_CLOCK, msg);
                }
                running = 1;
            }
            else if(buf[2] == 'T'){
                running = 0;
            }
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
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got invalid message\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        if (buf[4] == ' '){
            space_loc = 4;
            if (buf[3] < 48 || buf[3] > 57) { //NaN
                #ifdef DEBUG_0
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
                #ifdef DEBUG_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[0] = buf[3];
            if (buf[4] < 48 || buf[4] > 57) { //NaN
                #ifdef DEBUG_0
                    printf("ERROR! Set Process Proc got NaN for PID\r\n");
                #endif /* DEBUG_0 */
                continue;
            }
            buf_to_int[1] = buf[4];
            buf_to_int[2] = '\0';
        }
        if (space_loc+2 != strlen(buf)-2){
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got invalid chars\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        pid = atoi(buf_to_int);
        if (pid < 1 || pid > 13){ //should not allow change priority of NULL or I PROCS
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got restricted pid\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        if (buf[space_loc] != ' '){
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got invalid message\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        if (buf[space_loc+1] < 48 || buf[space_loc+1] > 57) { //NaN
            #ifdef DEBUG_0
                printf("ERROR! Set Process Proc got NaN for Priority\r\n");
            #endif /* DEBUG_0 */
            continue;
        }
        buf_to_int[0] = buf[space_loc + 1];
        buf_to_int[1] = '\0';
        priority = atoi(buf_to_int);
        if (priority < 0 || priority > 3){
            #ifdef DEBUG_0
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

    send_message(PID_KCD, msg);
    #ifdef DEBUG_0
            printf("Stress Proc A Loop 1\r\n");
    #endif
    while(1){
        #ifdef DEBUG_0
            printf("Enter %Z to exit loop, any other input outputs ERROR\r\n");
        #endif
        msg = receive_message(&sender_id);
        #ifdef DEBUG_0
            printf("Stress Porc A got a message from %d, and the message is %s\r\n", sender_id, msg->mtext);
        #endif
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
        if(num == 69){
            #ifdef DEBUG_0
                printf("almost reaches 30, might have problem here");
            #endif
        }
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
        #ifdef DEBUG_0
            if(sender_id == PID_A){
                printf("Stress Proc B got the message from PROC_A: %d, and the message is %s\r\n", sender_id, msg->mtext);
            }
        #endif
        
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
                send_message(PID_CRT,msg_p); //message p gets released after crt sends it to uart

                //hibernate for 10 sec
                //request msg and delayed send for 10 sec
                msg_q = (MSG_BUF *) request_memory_block();
                msg_q->mtype = WAKEUP10;
								strncpy(msg_q->mtext, "delay send\r\n", strlen("delay send\r\n")+1);
                delayed_send(PID_C, msg_q, TEN_SEC);
                while(1){
                    msg_p = (MSG_BUF *) receive_message(&sender_id);
                    #ifdef DEBUG_0
                        if(sender_id == PID_B){
                            printf("Stress Proc C got a message from PROC_B %d, and the message is %s\r\n", sender_id, msg_p->mtext);
                        }
                    #endif
                    if(msg_p->mtype == WAKEUP10){
                        release_memory_block(msg_p);    //release memory block that was requested for delay send;
                        break;
                    }
                    else{
                        msg_queue_enque(&msg_queue, msg_p);
                    }
                }
            }else {
         release_memory_block(msg_p); //Need to free msg_p if not going to uart 
      }
        }else{
				#ifndef DEBUG_0
					printf("PANIC!!!");
				#endif
        release_memory_block(msg_p); //Need to free msg_p if not going to uart 
    }
        
        //not releasing p here because uart proc will do it eventually
        release_processor();
    }


}
