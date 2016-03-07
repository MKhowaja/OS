/**
 * @brief: uart_irq.c 
 * @author: NXP Semiconductors
 * @author: Y. Huang
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "rtx.h"
#include "uart_polling.h"
#include "k_process.h"
#include "k_message.h"
#include "k_memory.h"
#include "string.h"
#include "k_rtx.h"
#ifdef DEBUG_0
#include "printf.h"
#endif


MSG_BUF* message = NULL;
uint8_t *gp_buffer = "\0"; //pointer to beginning of sent message from CRT
uint8_t g_char_in;
uint8_t g_char_out;
uint32_t buffer_index = 0;
uint32_t buffer_size = 200; //arbitrarily defined
uint8_t g_buffer[200];
extern uint32_t g_switch_flag;

extern int k_release_processor(void);
void reset_g_buffer(void);

/**
 * @brief: initialize the n_uart
 * NOTES: It only supports UART0. It can be easily extended to support UART1 IRQ.
 * The step number in the comments matches the item number in SectioMSG_BUFn 14.1 on pg 298
 * of LPC17xx_UM
 */
int uart_irq_init(int n_uart) {

	LPC_UART_TypeDef *pUart;
	reset_g_buffer();

	if ( n_uart ==0 ) {
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
		//LPC_SC->PCONP |= BIT(3);
	
		-----------------------------------------------------
		Step2: Select the clock source. 
		       Default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZin system_LPC17xx.c file.
		PCLK = CCLK/4, default setting after reset.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(7)|BIT(6));	
			
		-----------------------------------------------------
		Step 5: Pin Ctrl Block configuration for TXD and RXD
		        See Table 79 on pg108 in LPC17xx_UM.
		-----------------------------------------------------
		Note this is done before Steps3-4 for coding purpose.
		*/
		
		/* Pin P0.2 used as TXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 4);  
		
		/* Pin P0.3 used as RXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 6);  

		pUart = (LPC_UART_TypeDef *) LPC_UART0;	 
		
	} else if ( n_uart == 1) {
	    
		/* see Table 79 on pg108 in LPC17xx_UM */ 
		/* Pin P2.0 used as TXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 0);

		/* Pin P2.1 used as RXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 2);	      

		pUart = (LPC_UART_TypeDef *) LPC_UART1;
		
	} else {
		return 1; /* not supported yet */
	} 
	
	/*
	-----------------------------------------------------
	Step 3: Transmission Configuration.
	        See section 14.4.12.1 pg313-315 in LPC17xx_UM 
	        for baud rate calculation.
	-----------------------------------------------------
        */
	
	/* Step 3a: DLAB=1, 8N1 */
	pUart->LCR = UART_8N1; /* see uart.h file */ 

	/* Step 3b: 115200 baud rate @ 25.0 MHZ PCLK */
	pUart->DLM = 0; /* see table 274, pg302 in LPC17xx_UM */
	pUart->DLL = 9;	/* see table 273, pg302 in LPC17xx_UM */
	
	/* FR = 1.507 ~ 1/2, DivAddVal = 1, MulVal = 2
	   FR = 1.507 = 25MHZ/(16*9*115200)
	   see table 285 on pg312 in LPC_17xxUM
	*/
	pUart->FDR = 0x21;       
	
 

	/*
	----------------------------------------------------- 
	Step 4: FIFO setup.
	       see table 278 on pg305 in LPC17xx_UM
	-----------------------------------------------------
        enable Rx and Tx FIFOs, clear Rx and Tx FIFOs
	Trigger level 0 (1 char per interrupt)
	*/
	
	pUart->FCR = 0x07;

	/* Step 5 was done between step 2 and step 4 a few lines above */

	/*
	----------------------------------------------------- 
	Step 6 Interrupt setting and enabling
	-----------------------------------------------------
	*/
	/* Step 6a: 
	   Enable interrupt bit(s) wihtin the specific peripheral register.
           Interrupt Sources Setting: RBR, THRE or RX Line Stats
	   See Table 50 on pg73 in LPC17xx_UM for all possible UART0 interrupt sources
	   See Table 275 on pg 302 in LPC17xx_UM for IER setting 
	*/
	/* disable the Divisior Latch Access Bit DLAB=0 */
	pUart->LCR &= ~(BIT(7)); 
	
	//pUart->IER = IER_RBR | IER_THRE | IER_RLS; 
	pUart->IER = IER_RBR | IER_RLS;

	/* Step 6b: enable the UART interrupt from the system level */
	
	if ( n_uart == 0 ) {
		NVIC_EnableIRQ(UART0_IRQn); /* CMSIS function */
	} else if ( n_uart == 1 ) {
		NVIC_EnableIRQ(UART1_IRQn); /* CMSIS function */
	} else {
		return 1; /* not supported yet */
	}
	pUart->THR = '\0';
	return 0;
}


/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	CPSID i //disable irq
	PRESERVE8
	IMPORT c_UART0_IRQHandler
	IMPORT k_release_processor
	PUSH{r4-r11, lr}
	BL c_UART0_IRQHandler
	LDR R4, =__cpp(&g_switch_flag)
	LDR R4, [R4]
	MOV R5, #0     
	CMP R4, R5
	CPSIE i //enable irq
	BEQ  RESTORE    ; if g_switch_flag == 0, then restore the process that was interrupted
	BL k_release_processor  ; otherwise (i.e g_switch_flag == 1, then switch to the other process)
RESTORE
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void c_UART0_IRQHandler(void)
{
	uart_i_process();
}

void reset_g_buffer() {
    uint32_t i;

    buffer_index = 0;

    for (i = 0; i < buffer_size; i++) {
        g_buffer[i] = '\0';
    }
}

void set_g_buffer(char * input){
	//strcpy(g_buffer, input);
}

void uart_i_process(){
  uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	MSG_BUF* msg;
	//PCB* current_process;
	int sender_id;
	int ret = 0;
	PCB* prev_pcb_node = k_get_current_process();
	PCB* current_process;
	
	
//#ifdef DEBUG_0
//	uart1_put_string("Entering c_UART0_IRQHandler\n\r");
//#endif // DEBUG_0
	
	g_switch_flag = 0;
	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	
	if (IIR_IntId & IIR_RDA) { // interrupt Id and Receive Data Available
		/* keyboard input */
		g_char_in = pUart->RBR;
		//for \r \n and \0
		if ((buffer_index < buffer_size - 3) && (g_char_in != '\r')) {
			if (g_char_in != 8 && g_char_in != 127) { //not backspace or delete
				g_buffer[buffer_index++] = g_char_in;
      		}else {
				// if it's a backspace or delete, make index go back by 1
				if (buffer_index > 0) {
					g_buffer[buffer_index--] = '\0';
				}
			}

      		pUart->THR = g_char_in;
    	}else {
			//terminal characters
			g_buffer[buffer_index++] = '\r';
			pUart->THR = '\r';
			g_buffer[buffer_index++] = '\n';
			pUart->THR = '\n';
			g_buffer[buffer_index++] = '\0';
			pUart->THR = '\0';

			// prepare message to kcd to decode
			msg = (MSG_BUF*) k_request_memory_block_nonpreempt();
      		if (msg != NULL) {
			
						msg->mtype = DEFAULT;
		      	strncpy(msg->mtext, (char*)g_buffer, buffer_index);

						current_process = k_get_current_process();
						msg->sender_pid = current_process->m_pid;
						// send message to kcd
						k_send_message_nonpreempt(PID_KCD, msg);
			    
						g_switch_flag = 1;
						
		    }
				reset_g_buffer();
		}
		#ifdef _DEBUG_HOTKEYS
		if (( g_char_in == KEY_READY ||
                g_char_in == KEY_BLOCKED_MEM ||
                g_char_in == KEY_BLOCKED_MSG ||
                g_char_in == KEY_MSG_LOG) && buffer_index == 4) {

			printf("----------------------------------------------\r\n");
			print_current_process();
			switch (g_char_in) {
                case KEY_READY:
                    printf("READY QUEUE:\r\n");
                    print_queue(PRINT_READY);
                    break;

                case KEY_BLOCKED_MEM:
                    printf("MEM BLOCKED QUEUE:\r\n");
                    print_queue(PRINT_MEM_BLOCKED);
                    break;

                case KEY_BLOCKED_MSG:
                    printf("MESSAGE BLOCKED QUEUE:\r\n");
                    print_queue(PRINT_MSG_BLOCKED);
                    break;

                case KEY_MSG_LOG:
                	//TODO output 10 most recent sent and received messages
                    //handle
                    break;

                default:
                    break;
            }
		}
		printf("----------------------------------------------\r\n");
		#endif

		
	} else if (IIR_IntId & IIR_THRE) {
	// interrupt Id and THRE Interrupt, transmit holding register becomes empty
		// Should only get here from send message from CRT PROC
			if (message != NULL){
					ret = k_release_memory_block_nonpreempt((void*)message);
					if (ret == 1) {
							g_switch_flag = 1;
					}					
			}
			message = NULL;

      message = k_receive_message_nonblocking(&sender_id);
			if (message != NULL) {
          gp_buffer = (uint8_t*)message->mtext; //pointer to beginning of sent message from CRT
					while (*gp_buffer != '\0'){				
						g_char_out = *gp_buffer;
						pUart->THR = g_char_out; //prints to CRT
						gp_buffer++; 
					}
	  	} 
	  	else { //no message to receive
					#ifdef DEBUG_0
							printf("Did not receive any message from CRT!\r\n");
					#endif // DEBUG_0 
					pUart->IER ^= IER_THRE; // toggle the IER_THRE (interrupt) bit 
					pUart->THR = '\0'; //only send null terminal to CRT
			}
	} else {
			#ifdef DEBUG_0
					printf("Could not handle interrupt in uart_i_process!\r\n");
			#endif // DEBUG_0 
			return;
	}	
}
