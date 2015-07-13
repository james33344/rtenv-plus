#include "string.h"
#include "host.h"
#include "trace.h"
#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>


#define NVIC_INTERRUPTx_PRIORITY ( ( volatile unsigned char *) 0xE000E400 )
int logfile = 0;
extern int tick_count;
int prev_t = 0;

int get_interrupt_priority(int interrupt)
{
	if (interrupt < 240)
		return NVIC_INTERRUPTx_PRIORITY[interrupt];
	return -1;
}

unsigned int get_reload()
{
	return *(unsigned int *) 0xE000E014;
}

unsigned int get_current()
{
	return *(unsigned int *) 0xE000E018;
}

int __attribute__((naked)) get_current_interrupt_number()
{
	__asm volatile(
	    "mrs r0, ipsr\n"
	    "bx  lr     \n"
	);
}

int _snprintf_int(int num, char *buf, int buf_size)
{
	int len = 1;
	char *p;
	int i = num < 0 ? -num : num;

	for (; i >= 10; i /= 10, len++);

	if (num < 0)
		len++;

	i = num;
	p = buf + len - 1;
	do {
		if (p < buf + buf_size)
			*p-- = '0' + i % 10;
		i /= 10;
	} while (i != 0);

	if (num < 0)
		*p = '-';

	return len < buf_size ? len : buf_size;
}


int snprintf(char *buf, size_t size, const char *format, ...)
{
	va_list ap;
	char *dest = buf;
	char *last = buf + size;
	char ch;

	va_start(ap, format);
	for (ch = *format++; dest < last && ch; ch = *format++) {
		if (ch == '%') {
			ch = *format++;
			switch (ch) {
			case 's' : {
					char *str = va_arg(ap, char*);
					/* strncpy */
					while (dest < last) {
						if ((*dest = *str++))
							dest++;
						else
							break;
					}
				}
				break;
			case 'd' : {
					int num = va_arg(ap, int);
					dest += _snprintf_int(num, dest,
					                      last - dest);
				}
				break;
			case '%' :
				*dest++ = ch;
				break;
			default :
				return -1;
			}
		} else {
			*dest++ = ch;
		}
	}
	va_end(ap);

	if (dest < last)
		*dest = 0;
	else
		*--dest = 0;

	return dest - buf;
}

float get_time()
{
     unsigned int const *reload = (void *) 0xE000E014;
     unsigned int const *current = (void *) 0xE000E018;
 
	return (tick_count) + ((*reload - *current) / (*reload));
}


void trace_task_create(void *task,
                        const char *task_name,
						unsigned int priority)
{   
    char buf[128];
    int len = snprintf(buf, 128, "task %d %d %s\n", task, priority,
                        task_name);
	host_action(SYS_WRITE, logfile, buf, len);
}
 
void trace_task_switch(void *prev_task,
                        unsigned int prev_tick,
                        void *curr_task)
{   
	int sub = prev_tick - get_current();
    char buf[128];
	int len = snprintf(buf, 128, "switch %d %d %d %d %d %d %d\n",
                        prev_task, curr_task,
                        tick_count, get_reload(),
                        prev_tick, get_current(), sub);
	host_action(SYS_WRITE, logfile, buf, len);
}
 
void trace_create_mutex(void *mutex)
{
    char buf[128];
    int len = snprintf(buf, 128, "mutex %d %d\n", get_time(), mutex);
	host_action(SYS_WRITE, logfile, buf, len);
}

void trace_queue_create(void *queue,
                        int queue_type,
                        unsigned int queue_size)
{
	char buf[128];
	int len = snprintf(buf, 128, "queue create %d %d %d %d\n",
	                   get_time(), queue, queue_type, queue_size);
	host_action(SYS_WRITE, logfile, buf, len);
}

void trace_queue_send(void *task,
                      void *queue)
{
	char buf[128];
	int len = snprintf(buf, 128, "queue send %d %d %d\n",
	                   get_time(), task, queue);
	host_action(SYS_WRITE, logfile, buf, len);
}

void trace_queue_recv(void *task,
                      void *queue)
{
	char  buf[128];
	int len = snprintf(buf, 128, "queue recv %d %d %d\n",
	                   get_time(), task, queue);
	host_action(SYS_WRITE, logfile, buf, len);
}

void trace_queue_block(void *task,
                       void *queue)
{
	char buf[128];
	int len = snprintf(buf, 128, "queue block %d %d %d\n",
	                   get_time(), task, queue);
	host_action(SYS_WRITE, logfile, buf, len);
}

void trace_interrupt_in()
{
	prev_t = get_current();
	char buf[128];
	int number = get_current_interrupt_number();
	int len = snprintf(buf, 128, "interrupt in %d %d %d\n", prev_t,
	                   number, get_interrupt_priority(number));
	host_action(SYS_WRITE, logfile, buf, len);
}

void trace_interrupt_out()
{
	int t = get_current();
	char buf[128];
	int number = get_current_interrupt_number();
	int len = snprintf(buf, 128, "interrupt out %d %d\n",
	                   prev_t - t, number);
	host_action(SYS_WRITE, logfile, buf, len);
}

