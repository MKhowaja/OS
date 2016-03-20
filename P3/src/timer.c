/**
 * @brief timer.c - Timer example code. Tiemr IRQ is invoked every 1ms
 * @author T. Reidemeister
 * @author Y. Huang
 * @author NXP Semiconductors
 * @date 2012/02/12
 */

#include <LPC17xx.h>
#include "timer.h"
#include "k_message.h"
#include "k_rtx.h"
#ifdef DEBUG_0
	#include "printf.h"
#endif
#define BIT(X) (1<<X)

volatile uint32_t g_timer_count = 0; // increment every 1 ms
MSG_BUF* sorted_queue = NULL;
int send_flag;

/**
 * @brief: initialize timer. Only timer 0 is supported
 */

uint32_t timer_init(uint8_t n_timer) 
{
	LPC_TIM_TypeDef *pTimer;
	if (n_timer == 0) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		
		----------------------------------------------------- 
		Step 1: Power control configuration.
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(1);
	
		-----------------------------------------------------
		Step2: Select the clock source, 
		       default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZ in system_LPC17xx.c file.
		PCLK = CCLK/4, default setting in system_LPC17xx.c.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(3)|BIT(2));	

		-----------------------------------------------------
		Step 3: Pin Ctrl Block configuration. 
		        Optional, not used in this example
		        See Table 82 on pg110 in LPC17xx_UM 
		-----------------------------------------------------
		*/
		pTimer = (LPC_TIM_TypeDef *) LPC_TIM0;

	} else { /* other timer not supported yet */
		return 1;
	}

	/*
	-----------------------------------------------------
	Step 4: Interrupts configuration
	-----------------------------------------------------
	*/

	/* Step 4.1: Prescale Register PR setting 
	   CCLK = 100 MHZ, PCLK = CCLK/4 = 25 MHZ
	   2*(12499 + 1)*(1/25) * 10^(-6) s = 10^(-3) s = 1 ms
	   TC (Timer Counter) toggles b/w 0 and 1 every 12500 PCLKs
	   see MR setting below 
	*/
	pTimer->PR = 12499;  

	/* Step 4.2: MR setting, see section 21.6.7 on pg496 of LPC17xx_UM. */
	pTimer->MR0 = 1;

	/* Step 4.3: MCR setting, see table 429 on pg496 of LPC17xx_UM.
	   Interrupt on MR0: when MR0 mathches the value in the TC, 
	                     generate an interrupt.
	   Reset on MR0: Reset TC if MR0 mathches it.
	*/
	pTimer->MCR = BIT(0) | BIT(1);

	g_timer_count = 0;

	/* Step 4.4: CSMSIS enable timer0 IRQ */
	NVIC_EnableIRQ(TIMER0_IRQn);

	/* Step 4.5: Enable the TCR. See table 427 on pg494 of LPC17xx_UM. */
	pTimer->TCR = 1;

	return 0;
}

/**
 * @brief: use CMSIS ISR for TIMER0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_TIMER0_IRQHandler does the rest of irq handling
 */
__asm void TIMER0_IRQHandler(void)
{
	/*having problem here which disables irq
	in receive message*/
	
	CPSID i //disable irq
	PRESERVE8
	IMPORT c_TIMER0_IRQHandler
	PUSH{r4-r11, lr}
	BL c_TIMER0_IRQHandler
	CPSIE i //enable irq
	POP{r4-r11, pc}
	
} 
/**
 * @brief: c TIMER0 IRQ Handler
 */
void c_TIMER0_IRQHandler(void)
{
	/* ack inttrupt, see section  21.6.1 on pg 493 of LPC17XX_UM */
	LPC_TIM0->IR = BIT(0);
	g_timer_count++ ;
	timer_i_process();
}

void timer_i_process(void) {
	//PCB * current_process;
	//PCB *	receiver_process;
	MSG_BUF* message_to_send;
	int target_pid;
	//int send_flag = 0;
	send_flag = 0;
	// current_message = k_receive_message(NULL);
	// while (current_message!=NULL){
	// 	queue_add(&sorted_list, (queue_node*) current_message);
	// 	current_message = k_receive_message(NULL);
	// }
	
	//current_process = k_get_current_process();
	
	while (sorted_queue!= NULL && sorted_queue-> msg_delay <= g_timer_count){
		message_to_send = sorted_queue;
		sorted_queue = (MSG_BUF*)sorted_queue->mp_next;
		target_pid = message_to_send->receiver_pid;
		if(target_pid == 0){
			#ifdef DEBUG_0
				printf("WTF, why do we wanna send it to null proc, are you serious\n");
			#endif
		}
		message_to_send->mp_next = NULL;
		#ifdef DEBUG_0
                        printf("irq send a message to %d",target_pid);
        #endif
		send_flag = send_flag | k_send_message_nonpreempt(target_pid, (void*)message_to_send);
	}
	if (send_flag){
		__enable_irq();
		k_release_processor();
		__disable_irq();
	}
}

void expire_list_enqueue(MSG_BUF *msg){
	//absolute time
	MSG_BUF* message_iter = NULL;
	msg->msg_delay = g_timer_count + msg->msg_delay;
	msg->mp_next=NULL;
	if (sorted_queue == NULL) {
		sorted_queue = msg;
	} else{
		message_iter = sorted_queue;
		if (msg->msg_delay <= message_iter->msg_delay){
			msg->mp_next = message_iter;
			sorted_queue = msg;
		}
		else {
			while (message_iter->mp_next != NULL && msg->msg_delay > ((MSG_BUF*)(message_iter->mp_next))->msg_delay){
				message_iter = (MSG_BUF*)message_iter->mp_next;
			}
			if (message_iter == msg){
				#ifdef DEBUG_0
						printf("PANIC 2\r\n");
				#endif
			}
			msg->mp_next = message_iter->mp_next;
			message_iter->mp_next = msg;
		}
	}
	
	//queue_add(&sorted_list, (void*) msg);
}

uint32_t get_timer_count(void){
	return g_timer_count;
}