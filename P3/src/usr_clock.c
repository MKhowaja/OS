#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "string.h"
#include <stdlib.h>
#include "printf.h"

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
	//passed_test[5] = 1;
	
	g_test_procs[0].m_pid = PID_CLOCK;
  g_test_procs[0].m_priority = HIGH;
  g_test_procs[0].mpf_start_pc = &clock_proc;
	
	uart1_init();
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
