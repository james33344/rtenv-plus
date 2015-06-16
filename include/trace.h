#ifndef TRACE_H
#define TRACE_H

/* open for semihosting */
/* #define TRACE */
int get_interrupt_priority(int interrupt);

int snprintf(char *buf, size_t size, const char *format, ...);

unsigned int get_current();

void trace_task_create(void *task, const char *task_name, unsigned int priority);
void trace_task_switch(void *prev_task, unsigned int prev_tick, void *curr_task);

void trace_create_mutex(void *mutex);

void trace_queue_create(void *queue, int queue_type, unsigned int queue_size);
void trace_queue_send(void *task, void *queue);
void trace_queue_recv(void *task, void *queue);
void trace_queue_block(void *task, void *queue);

void trace_interrupt_in();
void trace_interrupt_in();
#endif
