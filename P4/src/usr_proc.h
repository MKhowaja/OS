/**
 * @file:   usr_proc.h
 * @brief:  Two user processes header file
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef USR_PROC_H_
#define USR_PROC_H

void set_test_procs(void);

void proc_p2_1(void);
void proc_p2_2(void);
void proc_p2_3(void);
void proc_p2_4(void);
void proc_p2_5(void);
void proc_p2_6(void);

void set_process_proc(void);
void test_proc_for_set_process_proc(void);

void stress_test_proc_a(void);
void stress_test_proc_b(void);
void stress_test_proc_c(void);


//wall clock user test process
void clock_proc(void);
void timer_test_proc(void);

#endif /* USR_PROC_H_ */
