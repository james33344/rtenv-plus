#include "kconfig.h"
#include "kernel.h"
#ifndef STM32F4
#include "stm32f10x.h"
#include "stm32_p103.h"
#else
#include "stm32f4xx.h"
#include "stm32_p103.h"
#endif
#include "RTOSConfig.h"
#include "syscall.h"
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include "string.h"
#include "pthread.h"
#include "task.h"
#include "memory-pool.h"
#include "path.h"
#include "pipe.h"
#include "fifo.h"
#include "mqueue.h"
#include "signal.h"
#include "block.h"
#include "romdev.h"
#include "event-monitor.h"
#include "romfs.h"
#include "trace.h"
#include "host.h"

extern int task_start();
extern int logfile;
unsigned int prev_tick = 0;
unsigned int prev_task;

extern int main();

unsigned int tick_count = 0;
int timeup = 0;

struct mutex_t {
    unsigned int addr[MUTEX_LIMIT];
    int count;
    int ishead;
} __mutex;


void write_blank(int blank_num);
size_t current_task = 0;
size_t task_count = 0;

struct task_control_block tasks[TASK_LIMIT];

struct sigaction sig[SIGNUM];

void signal_default(int arg) { /*	do nothing	*/
}


/* TODO
 * Error detect
 */
void signal_server() {
    int signum = 0;
    int mode = 0;
    int replyfd = -1;
    unsigned int func_addr = 0;

    while (1) {
        read(SIGSERVER_FD, &mode, sizeof(int));
        read(SIGSERVER_FD, &replyfd, sizeof(int));
        switch (mode) {
            case SIGSET: {
                int status = 0;
                read(SIGSERVER_FD, &signum, sizeof(int));
                read(SIGSERVER_FD, &func_addr, sizeof(void (*)(int)));
                sig[signum].sa_u.sa_handler = (void (*)(int)) func_addr;
                sig[signum].sa_flags = STATE_LEGAL;

                write(replyfd, &status, sizeof(int));
            } break;
            case SIGRAISE: {
                read(SIGSERVER_FD, &signum, sizeof(int));
                if (sig[signum].sa_flags == STATE_LEGAL) {
                    func_addr = (unsigned int) sig[signum].sa_u.sa_handler;
                }
                else {
                    func_addr = (unsigned int) SIG_ERR;
                }
                write(replyfd, &func_addr, sizeof(void (*)(int)));

            } break;
            case SIGDFL: {
                read(SIGSERVER_FD, &signum, sizeof(int));
                if (sig[signum].sa_flags == STATE_LEGAL) {
                    func_addr = (unsigned int) sig[signum].sa_u.sa_handler;
                    sig[signum].sa_u.sa_handler = signal_default;
                }
                else {
                    func_addr = (unsigned int) SIG_ERR;
                }
                write(replyfd, &func_addr, sizeof(void (*)(int)));

            } break;
            case SIGIGN: {
                read(SIGSERVER_FD, &signum, sizeof(int));
                if (sig[signum].sa_flags == STATE_LEGAL) {
                    func_addr = (unsigned int) sig[signum].sa_u.sa_handler;
                    sig[signum].sa_u.sa_handler = signal_default;
                    sig[signum].sa_flags = STATE_ILEGAL;
                }
                else {
                    func_addr = (unsigned int) SIG_ERR;
                }
                write(replyfd, &func_addr, sizeof(void (*)(int)));
            } break;
        }
    }
}

void mount_task() {
    mount("/dev/rom0", "/", ROMFS_TYPE, 0);
    task_exit(NULL);
}

int intr_release(struct event_monitor *monitor, int event,
                 struct task_control_block *task, void *data) {
    return 1;
}

int time_release(struct event_monitor *monitor, int event,
                 struct task_control_block *task, void *data) {
    int *tick_count = data;
    return task->stack->r0 == *tick_count;
}

int task_release(struct event_monitor *monitor, int event,
                 struct task_control_block *task, void *data) {
    int *pid = data;
    return tasks[*pid].inuse == 0;
}

int mutex_release(struct event_monitor *monitor, int event,
                  struct task_control_block *task, void *data) {
    if (__mutex.ishead == 1) {
        __mutex.ishead = 0;
        return 1;
    }
    return 0;
}

/*	user app entry	*/
void kernel_thread() {
    main();
    while (1)
        ;
}

/*
 *	main
 */

/* System resources */
unsigned int stacks[TASK_LIMIT][STACK_SIZE];
char memory_space[MEM_LIMIT];
struct file *files[FILE_LIMIT];
struct file_request requests[TASK_LIMIT];
struct list ready_list[PRIORITY_LIMIT + 1]; /* [0 ... 39] */
struct event events[EVENT_LIMIT];
struct task_control_block *current_tcb;
struct memory_pool memory_pool;
struct event_monitor event_monitor;
struct list *list;

int __rtenv_start() {
    int i;
#ifdef TRACE
    logfile = host_action(SYS_OPEN, "logqemu", 4);
#endif
    init_rs232();

    /* Initialize memory pool */
    memory_pool_init(&memory_pool, MEM_LIMIT, memory_space);

    /* Initialize all files */
    for (i = 0; i < FILE_LIMIT; i++)
        files[i] = NULL;

    /* Initialize ready lists */
    for (i = 0; i <= PRIORITY_LIMIT; i++)
        list_init(&ready_list[i]);

    /* Initialise event monitor */
    event_monitor_init(&event_monitor, events, ready_list);

    /* Initialize fifos */
    for (i = 0; i <= PATHSERVER_FD; i++)
        file_mknod(i, -1, files, S_IFIFO, &memory_pool, &event_monitor);

    /* Register IRQ events, see INTR_LIMIT */
    for (i = -15; i < INTR_LIMIT - 15; i++)
        event_monitor_register(&event_monitor, INTR_EVENT(i), intr_release, 0);

    /* Register TASK blocked event -> pthread_join, atomic etc. */
    for (i = 0; i < TASK_LIMIT; i++) {
        int pid = i;
        event_monitor_register(&event_monitor, TASK_EVENT(i), task_release,
                               &pid);
    }

    /* Register MUTEX events */
    for (i = 0; i < MUTEX_LIMIT; i++)
        event_monitor_register(&event_monitor, MUTEX_EVENT(i), mutex_release,
                               0);

    event_monitor_register(&event_monitor, TIME_EVENT, time_release,
                           &tick_count);
    /* Initialize all task threads */
    task_create(0, pathserver, NULL);
    task_create(0, signal_server, NULL);
    task_create(0, romdev_driver, NULL);
    task_create(0, romfs_server, NULL);
    task_create(0, mount_task, NULL);

    task_create(PRIORITY_LIMIT, kernel_thread, NULL);

    current_tcb = &tasks[current_task];

    __mutex.count = 0;
    __mutex.ishead = 1;

    SysTick_Config(configCPU_CLOCK_HZ / configTICK_RATE_HZ);

    task_start();

    /*	never execute here	*/
    return 0;
}

/*
 *	syscall handler
 */

void syscall_handler() {
    switch (current_tcb->stack->r7) {
        case 0x1: /* fork */
            if (task_count == TASK_LIMIT) {
                /* Cannot create a new task, return error */
                current_tcb->stack->r0 = -1;
            }
            else {
                /* Compute how much of the stack is used */
                size_t used = stacks[current_task] + STACK_SIZE -
                              (unsigned int *) current_tcb->stack;
                /* New stack is END - used */
                tasks[task_count].stack =
                    (void *) (stacks[task_count] + STACK_SIZE - used);
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
                list_push(&ready_list[tasks[task_count].priority],
                          &tasks[task_count].list);
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
                requests[current_task].buf = (void *) current_tcb->stack->r1;
                requests[current_task].size = current_tcb->stack->r2;
                current_tcb->stack->r0 = (int) &requests[current_task];

                /* Write */
                file_write(files[fd], &requests[current_task], &event_monitor);
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
                requests[current_task].buf = (void *) current_tcb->stack->r1;
                requests[current_task].size = current_tcb->stack->r2;
                current_tcb->stack->r0 = (int) &requests[current_task];

                /* Read */
                file_read(files[fd], &requests[current_task], &event_monitor);
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
            if (who > 0 && who < (int) task_count)
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
            value = (value < 0) ? 0 : ((value > PRIORITY_LIMIT) ? PRIORITY_LIMIT
                                                                : value);
            if (who > 0 && who < (int) task_count) {
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
            current_tcb->stack->r0 = file_mknod(
                current_tcb->stack->r0, current_tcb->pid, files,
                current_tcb->stack->r2, &memory_pool, &event_monitor);
            break;
        case 0x9: /* sleep */
            if (current_tcb->stack->r0 != 0) {
                current_tcb->stack->r0 += tick_count;
                event_monitor_block(&event_monitor, TIME_EVENT, current_tcb);
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
                current_tcb->stack->r0 = (int) &requests[current_task];

                /* Read */
                file_lseek(files[fd], &requests[current_task], &event_monitor);
            }
            else {
                current_tcb->stack->r0 = -1;
            }
        } break;
        case 0xb: /* task_block */
        {
            event_monitor_block(&event_monitor,
                                TASK_EVENT(current_tcb->stack->r0),
                                current_tcb);
            current_tcb->status = TASK_WAIT_TASK;
        } break;
        case 0xc: /* mutex_lock */
        {
            unsigned int mutex_addr = current_tcb->stack->r0;
            /* search if mutex exist */
            for (int i = 0; i < MUTEX_LIMIT; i++) {
                if (__mutex.addr[i] == mutex_addr) {
                    event_monitor_block(&event_monitor, MUTEX_EVENT(i),
                                        current_tcb);
                    current_tcb->status = TASK_WAIT_MUTEX;
                    current_tcb->stack->r0 = 0;
                    return;
                }
            }
            int empty_mutex = 0;

            for (; empty_mutex < MUTEX_LIMIT; empty_mutex++) {
                if (list_empty(
                        &event_monitor.events[MUTEX_EVENT(empty_mutex)].list)) {
                    break;
                }
            }
            event_monitor_block(&event_monitor, MUTEX_EVENT(empty_mutex),
                                current_tcb);
            current_tcb->status = TASK_WAIT_MUTEX;
            __mutex.addr[empty_mutex] = mutex_addr;
            __mutex.count++;
            current_tcb->stack->r0 = 0;

        } break;
        case 0xd: /* mutex_unlock */
        {
            unsigned int mutex_addr = current_tcb->stack->r0;
            /* search if mutex exist */
            for (int i = 0; i < MUTEX_LIMIT; i++) {
                if (__mutex.addr[i] == mutex_addr) {
                    event_monitor_release(&event_monitor, MUTEX_EVENT(i));
                    current_tcb->stack->r0 = 0;
                    __mutex.count--;
                    return;
                }
            }
            current_tcb->stack->r0 = -1;

        } break;
        default:
            break;
    }
}

void c_usart2_handler() {
    NVIC_DisableIRQ(USART2_IRQn);
    event_monitor_release(&event_monitor, INTR_EVENT(USART2_IRQn));
}

void c_systick_handler() {
    /* Never disable timer. We need it for pre-emption */
    timeup = 1;
    tick_count++;
    event_monitor_release(&event_monitor, TIME_EVENT);
}

void context_switch() {
    int i;
    struct task_control_block *task;
    /* Rearrange ready list and event list */
    event_monitor_serve(&event_monitor);
    /* Check whether to context switch */
    task = current_tcb;
    if (timeup && ready_list[task->priority].next == &task->list) {
        list_push(&ready_list[task->priority], &current_tcb->list);
        timeup = 0;
    }
    /* Select next TASK_READY task */
    for (i = 0; list_empty(&ready_list[i]); i++)
        ;

    list = ready_list[i].next;
    task = list_entry(list, struct task_control_block, list);
    current_task = task->pid;
    current_tcb = &tasks[current_task];
    __mutex.ishead = 1;
}

void __attribute__((naked)) set_pendsv() {
    __asm volatile(
        "ldr r4, =0xe000ed04	\n"
        /*	Pending PendSV */
        "mov r5, #0x12000000	\n"
        "str r5, [r4]			\n"
        "bx lr					\n");
}

void trace_pendsv_switch_prev() {
#ifdef TRACE
    prev_tick = get_current();
    prev_task = (unsigned int) current_tcb;
#endif
}

void trace_pendsv_switch_now() {
#ifdef TRACE
    trace_task_switch((void *) prev_task, prev_tick, current_tcb);
#endif
}
