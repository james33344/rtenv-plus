#ifndef KERNEL_H
#define KERNEL_H

#include "task.h"
#include "file.h"
#include "event-monitor.h"
#include <stddef.h>

#define INTR_EVENT(intr) (FILE_LIMIT + (intr) + 15) /* see INTR_LIMIT */
#define INTR_EVENT_REVERSE(event) ((event) - FILE_LIMIT - 15)
#define TIME_EVENT (FILE_LIMIT + INTR_LIMIT)
#define TASK_EVENT(pid) (TIME_EVENT + pid)
#define _disable_irq() __asm__ __volatile__ ( "cpsid i" );
#define _enable_irq() __asm__ __volatile__ ( "cpsie i" );

#endif
