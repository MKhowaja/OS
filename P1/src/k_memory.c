/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
	       /* stack grows down. Fully decremental stack */
linkedList free_list;
//U32 free_list[NUM_MEM];
U32* heap_start;
//U32* heap_end;
U32* test1;
U32* test2;

extern PCB* gp_current_process;

static node memory_node_pool[NUM_MEM];

static node* memory_node_factory(U32 * address){
	//can't just use m_pority here since all the process has the same prority, 
	//so that it will keep overriding the memory and all the nodes will have the same memory address
	int index = ((U32)address - (U32)heap_start) / (SZ_MEM_BLK);
	memory_node_pool[index].value = address;
	return &memory_node_pool[index];
}

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
	node * new_memory_node;
	U32 * temp_address;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_TOTAL_PROCS * sizeof(PCB *);
  
	for ( i = 0; i < NUM_TOTAL_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
	//p_end is 0x1000456C after above for loop
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \n", gp_pcbs[1]);
#endif
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
	//gp_stack is at RAM_END_ADDR
	linkedList_init(&free_list); // initialize linked list
	heap_start = (U32*)p_end;
	//heap_end = heap_start + (SZ_MEM_BLK/32) * NUM_MEM; //divide by 32 because pointer is already 32 bits and we need to alloc 128B
	for (i = 0; i < NUM_MEM; i++){
		temp_address = (heap_start + i * (SZ_MEM_BLK));
		new_memory_node = memory_node_factory(temp_address);
		linkedList_push_back(&free_list, new_memory_node);
	}
	#ifdef DEBUG_0  
	printf("Allocated heap done\n");
	#endif
	/* allocate memory for heap, not implemented yet*/
  
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack; 
	}
	return sp;
}

void *k_request_memory_block(void) {
	node* memory_node;
	void* memory_block;
#ifdef DEBUG_0 
	printf("k_request_memory_block: entering...\n");
#endif /* ! DEBUG_0 */
	// atomic ( on ) ;
	__disable_irq();
	// while ( no memory block is available ) {
	while(free_list.length == 0){
		// put PCB on b l o c k e d _ r e s o u r c e _ q ;
		gp_current_process->m_state = MEM_BLOCKED;
		// set process state to B L O C K E D _ O N _ R E S O U R C E ;
		k_release_processor();
	}
	// int mem_blk = next free block ;
	memory_node = linkedList_pop_front(&free_list);
	memory_block = memory_node->value;
	// update the heap ;
	
	// atomic ( off ) ;
	__enable_irq();
	return memory_block;
}

int k_release_memory_block(void *p_mem_blk) {
	int valid;
	node* free_memory_node;
#ifdef DEBUG_0 
	printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
	

// 	// atomic ( on ) ;
 	__disable_irq();

// 	// if ( memory block pointer is not valid )
// 	// return ERROR_CODE ;
 	valid = linkedList_contain(&free_list, p_mem_blk);
 	if (valid == 0){
 		return RTX_ERR;
 	}
 	// put memory_block into heap ;
 	free_memory_node = memory_node_factory( p_mem_blk);
 	linkedList_push_back(&free_list, free_memory_node);
	
	if(handle_blocked_process_ready()){
		k_release_processor();
	}

 // if ( blocked on resource q not empty ) {
 // h a n d l e _ p r o c e s s _ r e a d y ( pop ( blocked resource q ) ) ;
 // // + Check if any other process is in ready state be
 // }


// 	// atomic ( off ) ;
 	__enable_irq();
// 	// return SUCCESS_CODE
 	return RTX_OK;
}
