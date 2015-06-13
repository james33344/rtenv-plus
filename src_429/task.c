#include "task.h"
#include "kconfig.h"
#include "kernel.h"

#include "syscall.h"

#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include "string.h"
#include "task.h"
#include "memory-pool.h"
#include "path.h"
#include "pipe.h"
#include "fifo.h"
#include "mqueue.h"
#include "block.h"
#include "romdev.h"
#include "event-monitor.h"
#include "romfs.h"
#include <stddef.h>

/* System resources */
extern struct task_control_block tasks[TASK_LIMIT];
extern unsigned int stacks[TASK_LIMIT][STACK_SIZE];
extern struct list ready_list[PRIORITY_LIMIT + 1];  /* [0 ... 39] */
extern size_t task_count;
extern struct task_control_block *current_tcb;

int prv_priority = PRIORITY_DEFAULT;

unsigned int *init_task(unsigned int *stack, void (*start)())
{
	stack += STACK_SIZE - 18; /* End of stack, minus what we're about to push */
	stack[8] = (unsigned int)start;
	stack[16] = (unsigned int)start;
	stack[17] =	(unsigned int)0x01000000;
	return stack;
}

static int task_search_empty(){
	int i;
	for(i=0; i<TASK_LIMIT; i++){
		if(tasks[i].inuse == 0)
			break;
	}
	return i;
}

void task_create(int priority, void *func, void *arg){
	int task_pid;
	if(task_count == TASK_LIMIT || (task_pid = task_search_empty())==TASK_LIMIT){
		return;
	}	
    tasks[task_pid].stack = (void*)init_task(stacks[task_pid], func);
    tasks[task_pid].pid = task_pid;
    tasks[task_pid].priority = priority;
    tasks[task_pid].inuse = 1;
    list_init(&tasks[task_pid].list);
    list_push(&ready_list[tasks[task_pid].priority], &tasks[task_pid].list);
    task_count++;
}

void task_kill(int pid){
	_disable_irq();
	list_remove(&tasks[pid].list);
	/*	Never context switch here	*/
	tasks[pid].inuse = 0;
	task_count--;
	_enable_irq();

	while(1);
}

void task_exit(void* ptr){
	task_kill(current_tcb->pid);
}

