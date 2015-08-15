#include "pthread.h"
#include "kernel.h"
#include "kconfig.h"
#include "task.h"
#include "syscall.h"
#include "malloc.h"

#define get_pthread_attr_priority(attr) ({ \
	attr->sched_param.sched_priority; \
})

extern struct task_control_block* current_tcb;

int pthread_create(pthread_t *restrict thread,
					const pthread_attr_t *restrict attr,
			        void *(*start_routine)(void*), 
					void *restrict arg) {


	struct task_control_block* tcb;
	if (!attr) {
		tcb = task_create(PRIORITY_DEFAULT, start_routine, NULL);
	}
	else {
		if (attr->policy != ATTR_STATE_LEGAL) return EINVAL;
		tcb = task_create(get_pthread_attr_priority(attr), start_routine, NULL);
	}

	if (tcb == NULL) return EAGAIN;

	*thread = (pthread_t) malloc(sizeof(pthread_t));
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


/*
 * The pthread_attr_init() function shall initialize a thread attributes object attr 
 * with the default value for all of the individual attributes used by a given implementation.
 */ 
int pthread_attr_init(pthread_attr_t *attr) {
	/*TODO
	 * thread stack should have it's own range in linking script
	 * so that we can set stack size in task_create() to desired size
	 */ 
	/* TODO
	 * Add error return
	 * Ex: EINVAL
	 */ 
	attr->stack_size = 0;	
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	attr->sched_param.sched_priority = PRIORITY_DEFAULT;
	attr->policy = ATTR_STATE_LEGAL;

	return 0;
}


/* TODO
 * Add error return
 * Ex: EINVAL
 */
int pthread_attr_destroy(pthread_attr_t *attr) {
	
	attr->policy = ATTR_STATE_ILEGAL;
	return 0;	
}


/* TODO
 * error detect
 * Ex: EINVAL
 */
int pthread_attr_setschedparam(pthread_attr_t *restrict attr,
       const struct sched_param *restrict param) {
	attr->sched_param.policy = param->policy;
	attr->sched_param.sched_priority = param->sched_priority;

	return 0;
}

/* TODO
 * error detect
 * Ex: EINVAL
 */
int pthread_attr_getschedparam(const pthread_attr_t *restrict attr,
		       struct sched_param *restrict param) {
	param->policy = attr->sched_param.policy;
	param->sched_priority = attr->sched_param.sched_priority;

	return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {
	*detachstate = attr->detachstate;

	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
	attr->detachstate = detachstate;
	
	return 0;
}

