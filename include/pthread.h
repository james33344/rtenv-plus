#ifndef PTHREAD_H
#define PTHREAD_H

typedef struct {
	void * p;
	unsigned int x;
} ptw32_handle_t;

typedef ptw32_handle_t pthread_t;

enum {
/*
*
*pthread_attr_{get,set}detachstate
*
*/
	PTHREAD_CREATE_JOINABLE			= 0,  /* Default */
	PTHREAD_CREATE_DETACHED			= 1,

/*
*
*pthread_attr_{get,set}inheritsched
*
*/
	PTHREAD_INHERIT_SCHED			= 0,  /* Default */
	PTHREAD_EXPLICIT_SCHED			= 1,

/*
*
*pthread_{get,set}scope
*
*/
	PTHREAD_SCOPE_PROCESS			= 0,  /* Default */
	PTHREAD_SCOPE_SYSTEM			= 1,
}




#endif
