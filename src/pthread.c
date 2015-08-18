#include "pthread.h"
#include "kernel.h"
#include "kconfig.h"
#include "task.h"
#include "syscall.h"
#include "malloc.h"

/*	index = pid, value = address to pthread_t	*/
static pthread_t* __pthread_record[TASK_LIMIT];
extern struct task_control_block* current_tcb;

#define get_pthread_attr_priority(attr) ({ \
	attr->sched_param.sched_priority; \
})

#define is_thread_not_alive(thread) ({ \
	thread->released==0 ? 0 : 1;  \
})

#define is_thread_exit_but_not_released(thread) ({ \
	thread->tcb->inuse==0 ? 1 : 0;  \
})

static inline int is_attr_joinable(pthread_t* thread) {
	if((*thread)->attr && (*thread)->attr->detachstate == PTHREAD_CREATE_DETACHED) return 0;
	return 1;
}

static inline int is_thread_value_legal(pthread_t* thread) {
	if((*thread)->tcb == NULL) return 0;
	return 1;
}

static inline void __release_pthread(pthread_t* thread) {
	_disable_irq();
	(*thread)->released = 1;
	free(*thread);
	_enable_irq();

}

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
	(*thread)->released = 0;

	__pthread_record[tcb->pid] = thread; 

	return 0;
}

pthread_t pthread_self() {
	return *__pthread_record[current_tcb->pid];
}

int inline pthread_equal(pthread_t t1, pthread_t t2) {
	/* FIXME
	 * use better comparision
	 */
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

/* TODO
 * implement value_ptr
 */
int pthread_join(pthread_t thread, void **value_ptr) {

	if(!is_thread_value_legal(&thread)) return EINVAL;

	if(!is_attr_joinable(&thread)) return EINVAL;


	if(is_thread_not_alive(thread)) {
		return ESRCH;
	}
	else if (is_thread_exit_but_not_released(thread)) {
		__release_pthread(&thread);
		return 0;
	}

	task_block(thread->tcb->pid);
	
	__release_pthread(&thread);

	return 0;
}

int pthread_detach(pthread_t thread) {
	if(!is_attr_joinable(&thread)) return EINVAL;
	__release_pthread(&thread);	
	return 0;
}

/*
 * The pthread_attr_init() function shall initialize a thread attributes object attr 
 * with the default value for all of the individual attributes used by a given implementation.
 */ 
int pthread_attr_init(pthread_attr_t *attr) {
	/*TODO
	 * thread stack should have it's own range in linker script
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

