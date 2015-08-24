#ifndef SIGNAL_H
#define SIGNAL_H

#include "pthreadtypes.h"

typedef unsigned int pid_t;

typedef void (*sighandler_t)(int);
typedef volatile int sig_atomic_t;
typedef int sigset_t;

#define SIGSET 0
#define SIGDFL 1
#define SIGRAISE 2

#define SIG_DFL ((void(*)(int))1)
#define SIG_ERR ((void(*)(int))2)
#define SIG_HOLD ((void(*)(int))3)
#define SIG_IGN ((void(*)(int))4)

#define SIGABRT 0
#define SIGALRM 1
#define SIGBUS 2
#define SIGCHLD 3
#define SIGCONT 4
#define SIGFPE 5
#define SIGHUP 6
#define SIGILL 7
#define SIGINT 8
#define SIGKILL 9
#define SIGPIPE 10
#define SIGQUIT 11
#define SIGSEGV 12
#define SIGSTOP 13
#define SIGTERM 14
#define SIGTSTP 15
#define SIGTTIN 16
#define SIGTTOU 17
#define SIGUSR1 18
#define SIGUSR2 19
#define SIGTRAP 20
#define SIGURG 21
#define SIGXCPU 22
#define SIGXFSZ 23

#define SIGNUM 24

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

union sigval {
	int	sival_int;		/* Integer signal value */
	void* sival_ptr;	/* Pointer signal value */
}; 


#define SIGEV_NONE 0
#define SIGEV_SIGNAL 1
#define SIGEV_THREAD 2

struct sigevent {
	int sigev_notify;	/* Notification type */
	int sigev_signo;	/* Signal number */
	union sigval sigev_value;	/* Signal value */
	void (*signev_notify_function)(union sigval);	/* Notification function */
	pthread_attr_t* sigev_notify_attributes;	/* Notification function */
};

typedef struct {
	int si_signo;
	int si_code;
	int si_errno;
	pid_t si_pid;
	void* si_addr;
	int si_status;
	union sigval si_value;
} siginfo_t;

struct sigaction {
  union {
    void (*sa_handler) (int);
    void (*sa_sigaction) (int, siginfo_t *, void *);
  } sa_u;
  sigset_t sa_mask;
  int sa_flags;
};

void (*signal(int, void (*)(int)))(int);



int    kill(pid_t, int);
int    killpg(pid_t, int);
void   psiginfo(const siginfo_t *, const char *);
void   psignal(int, const char *);
int    pthread_kill(pthread_t, int);
int    pthread_sigmask(int, const sigset_t *restrict,
           sigset_t *restrict);
int    raise(int);
int    sigaction(int, const struct sigaction *restrict,
           struct sigaction *restrict);
int    sigaddset(sigset_t *, int);
//int    sigaltstack(const stack_t *restrict, stack_t *restrict);
int    sigdelset(sigset_t *, int);
int    sigemptyset(sigset_t *);
int    sigfillset(sigset_t *);
int    sighold(int);
int    sigignore(int);
int    siginterrupt(int, int);
int    sigismember(const sigset_t *, int);
int    sigpause(int);
int    sigpending(sigset_t *);
int    sigprocmask(int, const sigset_t *restrict, sigset_t *restrict);
int    sigqueue(pid_t, int, const union sigval);
int    sigrelse(int);
void (*sigset(int, void (*)(int)))(int);
int    sigsuspend(const sigset_t *);
//int    sigtimedwait(const sigset_t *restrict, siginfo_t *restrict,
//           const struct timespec *restrict);
int    sigwait(const sigset_t *restrict, int *restrict);
int    sigwaitinfo(const sigset_t *restrict, siginfo_t *restrict);


#endif


