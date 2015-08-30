#include "signal.h"
#include "rtenv.h"

#define write_sig_info_to_server(mode, self, sig) {\
		write(SIGSERVER_FD, &mode, sizeof(int)); \
		write(SIGSERVER_FD, &self, sizeof(int)); \
		write(SIGSERVER_FD, &sig, sizeof(int)); \
}

/* signal is a function accepting two arguments 
 * -- an int and a function pointer -- 
 *  and returning a function pointer.
 */
sighandler_t signal(int sig, sighandler_t func) {
	int mode = SIGSET;
	int status = -1;
	int self = getpid() + 3;
	unsigned int func_addr = (unsigned int) func;

	if (func == SIG_DFL || func == SIG_IGN){
		sighandler_t retval = SIG_ERR;	
		(func == SIG_DFL) ? (mode = SIGDFL) : (mode = SIGIGN);
		write_sig_info_to_server(mode, self, sig);
		read(self, &func_addr, sizeof(void(*)(int)));
			
		retval = (void(*)(int)) func_addr;

		return retval;
	}
	else {
		switch (sig) {
			case SIGCHLD: 
				write_sig_info_to_server(mode, self, sig);
				write(SIGSERVER_FD, &func_addr, sizeof(void(*)(int)));
				read(self, &status, sizeof(int));
				break;
			default:
				break;
		}	
	}
	return NULL;
}

int raise(int sig) {
	sighandler_t sighandle = NULL;
	unsigned int func_addr = 0;
	int self = getpid() + 3;
	int mode = SIGRAISE;

	write_sig_info_to_server(mode, self, sig);
	read(self, &func_addr, sizeof(void(*)(int)));

	sighandle = (void(*)(int)) func_addr;

	if (sighandle != SIG_ERR) {
		sighandle(sig);
		return 0;
	}
	return 1;

}

