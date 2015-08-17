#include "task.h"
#include "kconfig.h"
#include "kernel.h"

#include "syscall.h"

#include "erron.h"
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
#include "trace.h"

char str[5] = "test";

/* System resources */
extern struct task_control_block tasks[TASK_LIMIT];
extern unsigned int stacks[TASK_LIMIT][STACK_SIZE];
extern struct list ready_list[PRIORITY_LIMIT + 1];  /* [0 ... 39] */
extern size_t task_count;
extern struct task_control_block *current_tcb;
extern struct event_monitor event_monitor;

int prv_priority = PRIORITY_DEFAULT;

unsigned int *init_task(unsigned int *stack, void (*start)())
{
	stack += STACK_SIZE - 18; /* End of stack, minus what we're about to push */
	stack[8] = (unsigned int)start;
	stack[16] = (unsigned int)start;
	stack[17] =	(unsigned int)0x01000000;
	return stack;
}

static inline int task_search_empty(){
	int i;
	for(i=0; i<TASK_LIMIT; i++){
		if(tasks[i].inuse == 0) {
			break;
		}
	}
	return i;
}

/*	
 * TODO
 * Arguments passed to thread, pthread_create need too
 */
struct task_control_block* task_create(int priority, void *func, void *arg){
	int task_pid;
	if(task_count == TASK_LIMIT || (task_pid = task_search_empty())==TASK_LIMIT){
		return NULL;
	}	
    tasks[task_pid].stack = (void*)init_task(stacks[task_pid], func);
    tasks[task_pid].pid = task_pid;
    tasks[task_pid].priority = priority;
    tasks[task_pid].inuse = 1;
    list_init(&tasks[task_pid].list);
    list_push(&ready_list[tasks[task_pid].priority], &tasks[task_pid].list);
    task_count++;
#ifdef TRACE
	trace_task_create(&tasks[task_pid], str, priority);	
#endif

	return &tasks[task_pid];
}

int task_kill(int pid){
	if (!tasks[pid].inuse) return EINVAL;
	_disable_irq();
	list_remove(&tasks[pid].list);
	/*	Never context switch here	*/
	tasks[pid].inuse = 0;
	--task_count;
	event_monitor_release(&event_monitor, TASK_EVENT(pid));
	_enable_irq();

	return 0;
}

void task_exit(void* ptr){
	task_kill(current_tcb->pid);
	while(1);
}

void task_block(int pid) {
    event_monitor_block(&event_monitor,
                        TASK_EVENT(pid),
						current_tcb);
    current_tcb->status = TASK_WAIT_TASK;
	current_tcb->stack->r7 = 0xFFFF;
	__asm("svc 0");
}
