#include "pthread.h"
#include "kernel.h"
#include "kconfig.h"
#include "task.h"
#include "syscall.h"
#include "malloc.h"

extern struct task_control_block* current_tcb;

int pthread_create(pthread_t *restrict thread,
					const pthread_attr_t *restrict attr,
			        void *(*start_routine)(void*), 
					void *restrict arg) {

	*thread = (pthread_t) malloc(sizeof(pthread_t));

	struct task_control_block* tcb;
	if (!attr) {
		tcb = task_create(PRIORITY_DEFAULT, start_routine, NULL);
	}
	else {
		tcb = task_create(attr->sched_param.sched_priority, start_routine, NULL);
	}

	if (tcb == NULL) return EAGAIN;

	(*thread)->tcb = tcb;

	return 0;
}

pthread_t pthread_self() {
	pthread_t retval = (pthread_t) malloc(sizeof(pthread_t));
	retval->tcb = current_tcb;
	return retval;
}

int inline pthread_equal(pthread_t t1, pthread_t t2) {
	if(t1->tcb == t2->tcb) {
		/*	success	*/
		return 1;
	}
	return 0;
}


void inline pthread_exit(void *value_ptr) {
	task_exit(NULL);
}

int pthread_cancel(pthread_t thread) {
	if(!task_kill(thread->tcb->pid));
		return 0;
	return EINVAL;
}
