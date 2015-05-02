#include "task.h"
#include "kconfig.h"
#include "kernel.h"
#include "stm32f10x.h"
#include "stm32_p103.h"
#include "RTOSConfig.h"

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

extern unsigned int tick_count;

/* System resources */
extern struct task_control_block tasks[TASK_LIMIT];
extern unsigned int stacks[TASK_LIMIT][STACK_SIZE];
extern char memory_space[MEM_LIMIT];
extern struct file *files[FILE_LIMIT];
extern struct file_request requests[TASK_LIMIT];
extern struct list ready_list[PRIORITY_LIMIT + 1];  /* [0 ... 39] */
extern struct event events[EVENT_LIMIT];
extern size_t current_task;
extern size_t task_count;
extern struct task_control_block *current_tcb;
extern struct memory_pool memory_pool;
extern struct event_monitor event_monitor;
extern struct list *list;

int prv_priority = PRIORITY_DEFAULT;
unsigned int *init_task(unsigned int *stack, void (*start)())
{
	stack += STACK_SIZE - 18; /* End of stack, minus what we're about to push */
	stack[8] = (unsigned int)start;
	stack[16] = (unsigned int)start;
	stack[17] =	(unsigned int)0x01000000;
	return stack;
}

/*
void set_usart2_pendsv(){
	NVIC_ClearPendingIRQ(USART2_IRQn);
	__asm volatile(
		"ldr r3, =0xe000ed04	\n"
		"mov r2, #0x12000000	\n"
		"str r2, [r3]			\n"
	);
}
*/
void task_create(int priority, void *func){

    tasks[task_count].stack = (void*)init_task(stacks[task_count], func);
    tasks[task_count].pid = task_count;
    tasks[task_count].priority = priority;
    list_init(&tasks[task_count].list);
    list_push(&ready_list[tasks[task_count].priority], &tasks[task_count].list);
    task_count++;

}

void _context_switch(void){
		int i;
		struct task_control_block *task;
		int timeup;

		current_tcb->status = TASK_READY;
        timeup = 0;

        switch (current_tcb->stack->r7) {
        case 0x1: /* fork */
            if (task_count == TASK_LIMIT) {
                /* Cannot create a new task, return error */
                current_tcb->stack->r0 = -1;
            }
            else {
                /* Compute how much of the stack is used */
                size_t used = stacks[current_task] + STACK_SIZE
                          - (unsigned int*)current_tcb->stack;
                /* New stack is END - used */
                tasks[task_count].stack = (void*)(stacks[task_count] + STACK_SIZE - used);
                /* Copy only the used part of the stack */
                memcpy(tasks[task_count].stack, current_tcb->stack,
                       used * sizeof(unsigned int));
                /* Set PID */
                tasks[task_count].pid = task_count;
                /* Set priority, inherited from forked task */
                tasks[task_count].priority = current_tcb->priority;
				/* Set return values in each process */
                current_tcb->stack->r0 = task_count;
                tasks[task_count].stack->r0 = 0;
                list_init(&tasks[task_count].list);
                list_push(&ready_list[tasks[task_count].priority], &tasks[task_count].list);
                /* There is now one more task */
                task_count++;
            }
            break;
        case 0x2: /* getpid */
            current_tcb->stack->r0 = current_task;
            break;
        case 0x3: /* write */
            {
                /* Check fd is valid */
                int fd = current_tcb->stack->r0;
                if (fd < FILE_LIMIT && files[fd]) {
                    /* Prepare file request, store reference in r0 */
                    requests[current_task].task = current_tcb;
                    requests[current_task].buf =
                        (void*)current_tcb->stack->r1;
                    requests[current_task].size = current_tcb->stack->r2;
                    current_tcb->stack->r0 =
                        (int)&requests[current_task];

                    /* Write */
                    file_write(files[fd], &requests[current_task],
                               &event_monitor);

				}
                else {
                    current_tcb->stack->r0 = -1;
                }
            } break;
        case 0x4: /* read */
            {
                /* Check fd is valid */
                int fd = current_tcb->stack->r0;
                if (fd < FILE_LIMIT && files[fd]) {
                    /* Prepare file request, store reference in r0 */
                    requests[current_task].task = current_tcb;
                    requests[current_task].buf =
                        (void*)current_tcb->stack->r1;
                    requests[current_task].size = current_tcb->stack->r2;
                    current_tcb->stack->r0 =
                        (int)&requests[current_task];

                    /* Read */
                    file_read(files[fd], &requests[current_task],
                              &event_monitor);
                }
                else {
                    current_tcb->stack->r0 = -1;
                }
            } break;
        case 0x5: /* interrupt_wait */
            /* Enable interrupt */
            NVIC_EnableIRQ(current_tcb->stack->r0);
			/* Block task waiting for interrupt to happen */
            event_monitor_block(&event_monitor,
                                INTR_EVENT(current_tcb->stack->r0),
                                current_tcb);
            current_tcb->status = TASK_WAIT_INTR;
            break;
        case 0x6: /* getpriority */
            {
                int who = current_tcb->stack->r0;
                if (who > 0 && who < (int)task_count)
                    current_tcb->stack->r0 = tasks[who].priority;
                else if (who == 0)
                    current_tcb->stack->r0 = current_tcb->priority;
                else
                    current_tcb->stack->r0 = -1;
            } break;
        case 0x7: /* setpriority */
            {
                int who = current_tcb->stack->r0;
                int value = current_tcb->stack->r1;
                value = (value < 0) ? 0 : ((value > PRIORITY_LIMIT) ? PRIORITY_LIMIT : value);
                if (who > 0 && who < (int)task_count) {
                    tasks[who].priority = value;
				if (tasks[who].status == TASK_READY)
                        list_push(&ready_list[value], &tasks[who].list);
                }
                else if (who == 0) {
                    current_tcb->priority = value;
                    list_unshift(&ready_list[value], &current_tcb->list);
                }
                else {
                    current_tcb->stack->r0 = -1;
                    break;
                }
                current_tcb->stack->r0 = 0;
            } break;
        case 0x8: /* mknod */
            current_tcb->stack->r0 =
                file_mknod(current_tcb->stack->r0,
                           current_tcb->pid,
                           files,
                           current_tcb->stack->r2,
                           &memory_pool,
                           &event_monitor);
            break;
        case 0x9: /* sleep */
            if (current_tcb->stack->r0 != 0) {
				current_tcb->stack->r0 += tick_count;
                event_monitor_block(&event_monitor, TIME_EVENT,
                                    current_tcb);
                current_tcb->status = TASK_WAIT_TIME;
            }
            break;
        case 0xa: /* lseek */
            {
                /* Check fd is valid */
                int fd = current_tcb->stack->r0;
                if (fd < FILE_LIMIT && files[fd]) {
                    /* Prepare file request, store reference in r0 */
                    requests[current_task].task = current_tcb;
                    requests[current_task].buf = NULL;
                    requests[current_task].size = current_tcb->stack->r1;
                    requests[current_task].whence = current_tcb->stack->r2;
                    current_tcb->stack->r0 =
                        (int)&requests[current_task];

                    /* Read */
					file_lseek(files[fd], &requests[current_task],
								&event_monitor);
                }
                else {
                    current_tcb->stack->r0 = -1;
                }
            } break;
        default: /* Catch all interrupts */
            if ((int)current_tcb->stack->r7 < 0) {
                unsigned int intr = -current_tcb->stack->r7 - 16;

                if (intr == SysTick_IRQn) {
                    /* Never disable timer. We need it for pre-emption */
                    timeup = 1;
                    tick_count++;
                    event_monitor_release(&event_monitor, TIME_EVENT);
                }
                else {
                    /* Disable interrupt, interrupt_wait re-enables */
                    NVIC_DisableIRQ(intr);
                }
                event_monitor_release(&event_monitor, INTR_EVENT(intr));
            }
        }

        /* Rearrange ready list and event list */
        event_monitor_serve(&event_monitor);
		/* Check whether to context switch */
        task = current_tcb;
        if (timeup && ready_list[task->priority].next == &task->list)
            list_push(&ready_list[task->priority], &current_tcb->list);

        /* Select next TASK_READY task */
        for (i = 0; list_empty(&ready_list[i]); i++);

        list = ready_list[i].next;
        task = list_entry(list, struct task_control_block, list);
        current_task = task->pid;
        current_tcb = &tasks[current_task];
}


