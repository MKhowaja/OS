/**
 * @brief timer.h - Timer header file
 * @author Y. Huang
 * @date 2013/02/12
 */
#ifndef _TIMER_H_
#define _TIMER_H_
#include "sorted_queue.h"
#include "k_process.h"
#include <stdint.h>

extern uint32_t timer_init ( uint8_t n_timer );  /* initialize timer n_timer */
void timer_i_process(void);
void expire_list_enqueue(MSG_BUF *msg);
extern int handle_blocked_process_ready(PROC_STATE_E state);
uint32_t get_timer_count(void); /* getter of current time */

#endif /* ! _TIMER_H_ */
