#include "pthread.h"
#include "kernel.h"
#include "kconfig.h"
#include "task.h"
#include "syscall.h"



int pthread_create(pthread_t *restrict thread,
					const pthread_attr_t *restrict attr,
			        void *(*start_routine)(void*), 
					void *restrict arg) {
		
	task_create(0, start_routine, NULL);

	return 1;
}

pthread_t pthread_self() {
	return getpid();
}
