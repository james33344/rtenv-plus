#ifndef PTHREADTYPES_H
#define PTHREADTYPES_H

#include "task.h"

#define SIZEOF_PTHREAD_ATTR_T 36
#define SIZEOF_PTHREAD_MUTEX_T 24
#define SIZEOF_PTHREAD_MUTEXATTR_T 4
#define SIZEOF_PTHREAD_COND_T 48
#define SIZEOF_PTHREAD_COND_COMPAT_T 12
#define SIZEOF_PTHREAD_CONDATTR_T 4
#define SIZEOF_PTHREAD_RWLOCK_T 32
#define SIZEOF_PTHREAD_RWLOCKATTR_T 8
#define SIZEOF_PTHREAD_BARRIER_T 20
#define SIZEOF_PTHREAD_BARRIERATTR_T 4



//typedef unsigned long int pthread_t;


struct sched_param {
	int policy;
	int sched_priority;
};

typedef struct pthread_attr_struct {
	unsigned int stack_size;
	int detachstate;
	int policy;
	struct sched_param sched_param;
} pthread_attr_t;


struct pthread_struct {
	struct task_control_block* tcb;
	pthread_attr_t* attr;
	int released;
};

typedef struct pthread_struct* pthread_t;
//------------------------check to here--------------------

typedef struct pthread_internal_slist {
  struct pthread_internal_slist *next;
} pthread_slist_t;

/* Data structures for mutex handling.  The structure of the attribute
   type is not exposed on purpose.  */
typedef union {
	struct pthread_mutex_s {
		int lock;
		unsigned int count;
		int owner;
		/* KIND must stay at this position in the structure to maintain
		binary compatibility.  */
		int kind;
		unsigned int nusers;
		union {
			int spins;
			pthread_slist_t list;
		};
  } data;
		char size[SIZEOF_PTHREAD_MUTEX_T];
		long int align;
} pthread_mutex_t;

/* Mutex spins initializer used by PTHREAD_MUTEX_INITIALIZER.  */
#define PTHREAD_SPINS 0

typedef union {
  char size[SIZEOF_PTHREAD_MUTEXATTR_T];
  long int align;
} pthread_mutexattr_t;


/* Data structure for conditional variable handling.  The structure of
   the attribute type is not exposed on purpose.  */
typedef union {
	struct {
		int lock;
		unsigned int futex;
		unsigned long long int total_seq;
		unsigned long long int wakeup_seq;
		unsigned long long int woken_seq;
		void *mutex;
		unsigned int nwaiters;
		unsigned int broadcast_seq;
  } data;
		char size[SIZEOF_PTHREAD_COND_T];
		long long int align;
} pthread_cond_t;

typedef union {
  char size[SIZEOF_PTHREAD_CONDATTR_T];
  long int align;
} pthread_condattr_t;


/* Keys for thread-specific data */
typedef unsigned int pthread_key_t;


/* Once-only execution */
typedef int pthread_once_t;


#if defined USE_UNIX98 || defined USE_XOPEN2K
/* Data structure for read-write lock variable handling.  The
   structure of the attribute type is not exposed on purpose.  */
typedef union
{
  struct
  {
    int lock;
    unsigned int nr_readers;
    unsigned int readers_wakeup;
    unsigned int writer_wakeup;
    unsigned int nr_readers_queued;
    unsigned int nr_writers_queued;
    /* FLAGS must stay at this position in the structure to maintain
       binary compatibility.  */
    unsigned char flags;
    unsigned char shared;
    unsigned char pad1;
    unsigned char pad2;
    int writer;
  } data;
  char size[SIZEOF_PTHREAD_RWLOCK_T];
  long int align;
} pthread_rwlock_t;

#define PTHREAD_RWLOCK_ELISION_EXTRA 0

typedef union
{
  char size[SIZEOF_PTHREAD_RWLOCKATTR_T];
  long int align;
} pthread_rwlockattr_t;
#endif


#ifdef USE_XOPEN2K
/* POSIX spinlock data type.  */
typedef volatile int pthread_spinlock_t;


/* POSIX barriers data type.  The structure of the type is
   deliberately not exposed.  */
typedef union
{
  char size[SIZEOF_PTHREAD_BARRIER_T];
  long int align;
} pthread_barrier_t;

typedef union
{
  char size[SIZEOF_PTHREAD_BARRIERATTR_T];
  int align;
} pthread_barrierattr_t;
#endif


#endif

