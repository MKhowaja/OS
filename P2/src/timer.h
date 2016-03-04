/**
 * @brief timer.h - Timer header file
 * @author Y. Huang
 * @date 2013/02/12
 */
#ifndef _TIMER_H_
#define _TIMER_H_
#include "sorted_queue.h"

extern uint32_t timer_init ( uint8_t n_timer );  /* initialize timer n_timer */
void timer_i_process(void);
void expire_list_queue(MSG_T *msg);

#endif /* ! _TIMER_H_ */
