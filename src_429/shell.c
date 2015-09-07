#include "rtenv.h"
#include "romdev.h"

#define MAX_CMDNAME 19
#define MAX_ARGC 19
#define MAX_CMDHELP 1023
#define HISTORY_COUNT 8
#define CMDBUF_SIZE 64
#define MAX_ENVCOUNT 16
#define MAX_ENVNAME 15
#define MAX_ENVVALUE 63

extern struct task_control_block tasks[TASK_LIMIT];
void itoa(int n, char *dst, int base);

char keyup[4] = {'\x1b', '\x5b', '\x41', '\0'};
char keydown[4] = {'\x1b', '\x5b', '\x42', '\0'};
int key_count = 0;
char next_line[3] = {'\n','\r','\0'};
char cmd[HISTORY_COUNT][CMDBUF_SIZE];
int cur_his=0;
int fdout;
int fdin;


void check_keyword();
void find_events();
int fill_arg(char *const dest, const char *argv);
void itoa(int n, char *dst, int base);
void write_blank(int blank_num);

struct romfs_entry {
	  uint32_t parent;
		uint32_t prev;
		uint32_t next;
		uint32_t isdir;
		uint32_t len;
		uint8_t name[PATH_MAX];
};

/* Command handlers. */
void export_envvar(int argc, char *argv[]);
void show_echo(int argc, char *argv[]);
void show_cmd_info(int argc, char *argv[]);
void show_task_info(int argc, char *argv[]);
void show_man_page(int argc, char *argv[]);
void show_history(int argc, char *argv[]);
void show_xxd(int argc, char *argv[]);
void show_pwd(int argc, char *argv[]);
void show_ls(int argc, char *argv[]);
void show_cat(int argc, char *argv[]);
void show_keyup();


/* Enumeration for command types. */
enum {
	CMD_ECHO = 0,
	CMD_EXPORT,
	CMD_HELP,
	CMD_HISTORY,
	CMD_MAN,
	CMD_PS,
	CMD_XXD,
	CMD_PWD,
	CMD_LS,
	CMD_CAT,
	CMD_COUNT
} CMD_TYPE;
/* Structure for command handler. */
typedef struct {
	char cmd[MAX_CMDNAME + 1];
	void (*func)(int, char**);
	char description[MAX_CMDHELP + 1];
} hcmd_entry;
const hcmd_entry cmd_data[CMD_COUNT] = {
	[CMD_ECHO] = {.cmd = "echo", .func = show_echo, .description = "Show words you input."},
	[CMD_EXPORT] = {.cmd = "export", .func = export_envvar, .description = "Export environment variables."},
	[CMD_HELP] = {.cmd = "help", .func = show_cmd_info, .description = "List all commands you can use."},
	[CMD_HISTORY] = {.cmd = "history", .func = show_history, .description = "Show latest commands entered."}, 
	[CMD_MAN] = {.cmd = "man", .func = show_man_page, .description = "Manual pager."},
	[CMD_PS] = {.cmd = "ps", .func = show_task_info, .description = "List all the processes."},
	[CMD_XXD] = {.cmd = "xxd", .func = show_xxd, .description = "Make a hexdump."},
	[CMD_PWD] = {.cmd = "pwd", .func = show_pwd, .description = "Show current mount point."},
	[CMD_LS] = {.cmd = "ls", .func = show_ls, .description = "Show files."},
	[CMD_CAT] = {.cmd = "cat", .func = show_cat, .description = "Show files content."},
};

/* Structure for environment variables. */
typedef struct {
	char name[MAX_ENVNAME + 1];
	char value[MAX_ENVVALUE + 1];
} evar_entry;
evar_entry env_var[MAX_ENVCOUNT];
int env_count = 0;


void serialout(void* arg)
{
	int fd;
	char c;
	int doread = 1;
	mkfifo("/dev/tty0/out", 0);
	fd = open("/dev/tty0/out", 0);

	while (1) {
		if (doread)
			read(fd, &c, 1);
		doread = 0;
//		if (USART_GetFlagStatus(uart, USART_FLAG_TXE) == SET) {
		if (USART_GetFlagStatus((USART_TypeDef *)USART1, USART_FLAG_TXE) == SET) {
//			USART_SendData(uart, c);
			USART_SendData((USART_TypeDef *)USART1, c);
			doread = 1;
		}
	}
}

void serialin(void* arg)
{
	int fd;
	char c;
	mkfifo("/dev/tty0/in", 0);
	fd = open("/dev/tty0/in", 0);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	while (1) {
//		interrupt_wait(intr);
		interrupt_wait(USART1_IRQn);
		
/*		if (USART_GetFlagStatus(uart, USART_FLAG_RXNE) == SET) {
			c = USART_ReceiveData(uart);
*/	
		if (USART_GetFlagStatus((USART_TypeDef *)USART1, USART_FLAG_RXNE) == SET) {
			c = USART_ReceiveData((USART_TypeDef *)USART1);
			write(fd, &c, 1);
		}
	}
}


void echo()
{
	int fdout;
	int fdin;
	char c;

	fdout = open("/dev/tty0/out", 0);
	fdin = open("/dev/tty0/in", 0);

	while (1) {
		read(fdin, &c, 1);
		write(fdout, &c, 1);
	}
}

void rs232_xmit_msg_task(void* arg)
{
	int fdout;
	int fdin;
	char str[100];
	int curr_char;

	fdin = mq_open("/tmp/mqueue/out", 0);
	fdout = open("/dev/tty0/out", 0);

	while (1) {
		/* Read from the queue.  Keep trying until a message is
		 * received.  This will block for a period of time (specified
		 * by portMAX_DELAY). */
		read(fdin, str, 100);

		/* Write each character of the message to the RS232 port. */
		curr_char = 0;
		while (str[curr_char] != '\0') {
			write(fdout, &str[curr_char], 1);
			curr_char++;
		}
	}
}

void queue_str_task(const char *str, int delay)
{
	int fdout = mq_open("/tmp/mqueue/out", 0);
	int msg_len = strlen(str) + 1;

	while (1) {
		/* Post the message.  Keep on trying until it is successful. */
		write(fdout, str, msg_len);

		/* Wait. */
		sleep(delay);
	}
}

void queue_str_task1()
{
	queue_str_task("Hello 1\n", 200);
}

void queue_str_task2()
{
	queue_str_task("Hello 2\n", 50);
}

void serial_readwrite_task()
{
	int fdout;
	int fdin;
	char str[100];
	char ch;
	int curr_char;
	int done;

	fdout = mq_open("/tmp/mqueue/out", 0);
	fdin = open("/dev/tty0/in", 0);

	/* Prepare the response message to be queued. */
	memcpy(str, "Got:", 4);

	while (1) {
		curr_char = 4;
		done = 0;
		do {
			/* Receive a byte from the RS232 port (this call will
			 * block). */
			read(fdin, &ch, 1);

			/* If the byte is an end-of-line type character, then
			 * finish the string and inidcate we are done.
			 */
			if (curr_char >= 98 || ch == '\r' || ch == '\n') {
				str[curr_char] = '\n';
				str[curr_char+1] = '\0';
				done = -1;
			}
			/* Otherwise, add the character to the
			 * response string. */
			else
				str[curr_char++] = ch;
		} while (!done);

		/* Once we are done building the response string, queue the
		 * response to be sent to the RS232 port.
		 */
		write(fdout, str, curr_char+1 + 1);
	}
}

void serial_test_task(void* arg)
{
	char put_ch[2]={'0','\0'};
	char hint[] =  USER_NAME "@" USER_NAME "-STM32:~$ ";
	int hint_length = sizeof(hint);
	char *p = NULL;

	fdout = mq_open("/tmp/mqueue/out", O_CREAT);
	fdin = open("/dev/tty0/in", 0);

	for (;; cur_his = (cur_his + 1) % HISTORY_COUNT) {
		p = cmd[cur_his];
		write(fdout, hint, hint_length);

		while (1) {
			read(fdin, put_ch, 1);
			key_count++;
			if (put_ch[0] == '\r' || put_ch[0] == '\n') {
				*p = '\0';
				write(fdout, next_line, 3);
				break;
			}
			else if (put_ch[0] == 127 || put_ch[0] == '\b') {
				if (p > cmd[cur_his] && key_count) {
					p--;
					write(fdout, "\b \b", 4);
					key_count--;
				}
			}
			else if (p - cmd[cur_his] < CMDBUF_SIZE - 1) {
				*p++ = put_ch[0];
				write(fdout, put_ch, 2);
				/*	press keyboard 'up' show last history	*/
				if(key_count > 2 && !strncmp(p-3, keyup, 3)){
					write(fdout, keydown, 4);
					key_count -= 3;
					show_keyup();
					p = cmd[cur_his] + key_count;
					continue;
				}
			}
		}
		key_count = 0;
		check_keyword();	
	}
}

/* Split command into tokens. */
char *cmdtok(char *cmd)
{
	static char *cur = NULL;
	static char *end = NULL;
	if (cmd) {
		char quo = '\0';
		cur = cmd;
		for (end = cmd; *end; end++) {
			if (*end == '\'' || *end == '\"') {
				if (quo == *end)
					quo = '\0';
				else if (quo == '\0')
					quo = *end;
				*end = '\0';
			}
			else if (isspace((int)*end) && !quo)
				*end = '\0';
		}
	}
	else
		for (; *cur; cur++)
			;

	for (; *cur == '\0'; cur++)
		if (cur == end) return NULL;
	return cur;
}

void check_keyword()
{
	char *argv[MAX_ARGC + 1] = {NULL};
	char cmdstr[CMDBUF_SIZE];
	int argc = 1;
	int i;

	find_events();
	fill_arg(cmdstr, cmd[cur_his]);
	argv[0] = cmdtok(cmdstr);
	if (!argv[0])
		return;

	while (1) {
		argv[argc] = cmdtok(NULL);
		if (!argv[argc])
			break;
		argc++;
		if (argc >= MAX_ARGC)
			break;
	}

	for (i = 0; i < CMD_COUNT; i++) {
		if (!strcmp(argv[0], cmd_data[i].cmd)) {
			cmd_data[i].func(argc, argv);
			break;
		}
	}
	if (i == CMD_COUNT) {
		write(fdout, argv[0], strlen(argv[0]) + 1);
		write(fdout, ": command not found", 20);
		write(fdout, next_line, 3);
	}
}

void find_events()
{
	char buf[CMDBUF_SIZE];
	char *p = cmd[cur_his];
	char *q;
	int i;

	for (; *p; p++) {
		if (*p == '!') {
			q = p;
			while (*q && !isspace((int)*q))
				q++;
			for (i = cur_his + HISTORY_COUNT - 1; i > cur_his; i--) {
				if (!strncmp(cmd[i % HISTORY_COUNT], p + 1, q - p - 1)) {
					strcpy(buf, q);
					strcpy(p, cmd[i % HISTORY_COUNT]);
					p += strlen(p);
					strcpy(p--, buf);
					break;
				}
			}
		}
	}
}

char *find_envvar(const char *name)
{
	int i;

	for (i = 0; i < env_count; i++) {
		if (!strcmp(env_var[i].name, name))
			return env_var[i].value;
	}

	return NULL;
}

/* Fill in entire value of argument. */
int fill_arg(char *const dest, const char *argv)
{
	char env_name[MAX_ENVNAME + 1];
	char *buf = dest;
	char *p = NULL;

	for (; *argv; argv++) {
		if (isalnum((int)*argv) || *argv == '_') {
			if (p)
				*p++ = *argv;
			else
				*buf++ = *argv;
		}
		else { /* Symbols. */
			if (p) {
				*p = '\0';
				p = find_envvar(env_name);
				if (p) {
					strcpy(buf, p);
					buf += strlen(p);
					p = NULL;
				}
			}
			if (*argv == '$')
				p = env_name;
			else
				*buf++ = *argv;
		}
	}
	if (p) {
		*p = '\0';
		p = find_envvar(env_name);
		if (p) {
			strcpy(buf, p);
			buf += strlen(p);
		}
	}
	*buf = '\0';

	return buf - dest;
}

//export
void export_envvar(int argc, char *argv[])
{
	char *found;
	char *value;
	int i;

	for (i = 1; i < argc; i++) {
		value = argv[i];
		while (*value && *value != '=')
			value++;
		if (*value)
			*value++ = '\0';
		found = find_envvar(argv[i]);
		if (found)
			strcpy(found, value);
		else if (env_count < MAX_ENVCOUNT) {
			strcpy(env_var[env_count].name, argv[i]);
			strcpy(env_var[env_count].value, value);
			env_count++;
		}
	}
}

//ps
void show_task_info(int argc, char* argv[])
{
	char ps_message[]="PID STATUS PRIORITY";
	int ps_message_length = sizeof(ps_message);
	int task_i;

	write(fdout, &ps_message , ps_message_length);
	write(fdout, &next_line , 3);

	for (task_i = 0; task_i < TASK_LIMIT; task_i++) {
		if(tasks[task_i].inuse==0)continue;
		char task_info_pid[2];
		char task_info_status[2];
		char task_info_priority[3];

		task_info_pid[0]='0'+tasks[task_i].pid;
		task_info_pid[1]='\0';
		task_info_status[0]='0'+tasks[task_i].status;
		task_info_status[1]='\0';			

		itoa(tasks[task_i].priority, task_info_priority, 10);

		write(fdout, &task_info_pid , 2);
		write_blank(3);
			write(fdout, &task_info_status , 2);
		write_blank(5);
		write(fdout, &task_info_priority , 3);

		write(fdout, &next_line , 3);
	}
}


//help
void show_cmd_info(int argc, char* argv[])
{
	const char help_desp[] = "This system has commands as follow\n\r\0";
	int i;

	write(fdout, &help_desp, sizeof(help_desp));
	for (i = 0; i < CMD_COUNT; i++) {
		write(fdout, cmd_data[i].cmd, strlen(cmd_data[i].cmd) + 1);
		write(fdout, ": ", 3);
		write(fdout, cmd_data[i].description, strlen(cmd_data[i].description) + 1);
		write(fdout, next_line, 3);
	}
}

//echo
void show_echo(int argc, char* argv[])
{
	const int _n = 1; /* Flag for "-n" option. */
	int flag = 0;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-n"))
			flag |= _n;
		else
			break;
	}

	for (; i < argc; i++) {
		write(fdout, argv[i], strlen(argv[i]) + 1);
		if (i < argc - 1)
			write(fdout, " ", 2);
	}

	if (~flag & _n)
		write(fdout, next_line, 3);
}

//man
void show_man_page(int argc, char *argv[])
{
	int i;

	if (argc < 2)
		return;

	for (i = 0; i < CMD_COUNT && strcmp(cmd_data[i].cmd, argv[1]); i++)
		;

	if (i >= CMD_COUNT)
		return;

	write(fdout, "NAME: ", 7);
	write(fdout, cmd_data[i].cmd, strlen(cmd_data[i].cmd) + 1);
	write(fdout, next_line, 3);
	write(fdout, "DESCRIPTION: ", 14);
	write(fdout, cmd_data[i].description, strlen(cmd_data[i].description) + 1);
	write(fdout, next_line, 3);
}

void show_history(int argc, char *argv[])
{
	int i;

	for (i = cur_his + 1; i <= cur_his + HISTORY_COUNT; i++) {
		if (cmd[i % HISTORY_COUNT][0]) {
			write(fdout, cmd[i % HISTORY_COUNT], strlen(cmd[i % HISTORY_COUNT]) + 1);
			write(fdout, next_line, 3);
		}
	}
}

void write_blank(int blank_num)
{
	char blank[] = " ";
	int blank_count = 0;

	while (blank_count <= blank_num) {
		write(fdout, blank, sizeof(blank));
		blank_count++;
	}
}

char hexof(int dec)
{
    const char hextab[] = "0123456789abcdef";

    if (dec < 0 || dec > 15)
        return -1;

    return hextab[dec];
}

char char_filter(char c, char fallback)
{
    if (c < 0x20 || c > 0x7E)
        return fallback;

    return c;
}

#define XXD_WIDTH 0x10

//xxd
void show_xxd(int argc, char *argv[])
{
    int readfd = -1;
    char buf[XXD_WIDTH];
    char ch;
    char chout[2] = {0};
    int pos = 0;
    int size;
    int i;

    if (argc == 1) { /* fallback to stdin */
        readfd = fdin;
    }
    else { /* open file of argv[1] */
        readfd = open(argv[1], 0);

        if (readfd < 0) { /* Open error */
            write(fdout, "xxd: ", 6);
            write(fdout, argv[1], strlen(argv[1]) + 1);
            write(fdout, ": No such file or directory\r\n", 31);
            return;
        }
    }

    lseek(readfd, 0, SEEK_SET);
    while ((size = read(readfd, &ch, sizeof(ch))) && size != -1) {
        if (ch != -1 && ch != 0x04) { /* has something read */

            if (pos % XXD_WIDTH == 0) { /* new line, print address */
                for (i = sizeof(pos) * 8 - 4; i >= 0; i -= 4) {
                    chout[0] = hexof((pos >> i) & 0xF);
                    write(fdout, chout, 2);
                }

                write(fdout, ":", 2);
            }

            if (pos % 2 == 0) { /* whitespace for each 2 bytes */
                write(fdout, " ", 2);
            }

            /* higher bits */
            chout[0] = hexof(ch >> 4);
            write(fdout, chout, 2);

            /* lower bits*/
            chout[0] = hexof(ch & 0xF);
            write(fdout, chout, 2);

            /* store in buffer */
            buf[pos % XXD_WIDTH] = ch;

            pos++;

            if (pos % XXD_WIDTH == 0) { /* end of line */
                write(fdout, "  ", 3);

                for (i = 0; i < XXD_WIDTH; i++) {
                    chout[0] = char_filter(buf[i], '.');
                    write(fdout, chout, 2);
                }

                write(fdout, "\r\n", 3);
            }
        }
        else { /* EOF */
            break;
        }
    }

    if (pos % XXD_WIDTH != 0) { /* rest */
        /* align */
        for (i = pos % XXD_WIDTH; i < XXD_WIDTH; i++) {
            if (i % 2 == 0) { /* whitespace for each 2 bytes */
                write(fdout, " ", 2);
            }
            write(fdout, "  ", 3);
        }

        write(fdout, "  ", 3);

        for (i = 0; i < pos % XXD_WIDTH; i++) {
            chout[0] = char_filter(buf[i], '.');
            write(fdout, chout, 2);
        }

        write(fdout, "\r\n", 3);
    }
}


void show_keyup(){

	int i = (cur_his==0 ? HISTORY_COUNT - 1 : cur_his - 1);
	int j;
	if (cmd[i][0]) {
		for(j=0; j < key_count; j++){
			write(fdout, "\b \b", 4);
		}
		key_count = strlen(cmd[i]);
		write(fdout, cmd[i], key_count + 1);
		strcpy(cmd[cur_his], "");

		cur_his = i;
	}
}

void show_pwd(int argc, char *argv[]) {
	char buf[] = ROMDEV_PATH;
	write(fdout, buf, sizeof(buf));
	write(fdout, next_line, 3);
}

void show_ls(int argc, char *argv[]) {
	int fd = open("/dev/rom0", 0);
	struct romfs_entry {
		  uint32_t parent;
			uint32_t prev;
			uint32_t next;
			uint32_t isdir;
			uint32_t len;
			uint8_t name[PATH_MAX];
	};
	struct romfs_entry entry;
	/* root */
	read(fd, &entry, sizeof(entry));

	int root_len = entry.len - sizeof(entry);
	while(root_len > 0) {
		sleep(1);
		/* FIXME
		 * Memmory access error (0x6XXXXXX)
		 * So ls not work
		 */
		read(fd, &entry, sizeof(entry));
		if(entry.isdir) {
			write(fdout, "/", 2);
			write(fdout, entry.name, PATH_MAX);
			write(fdout, " ", 2);
		}
		else {
			write(fdout, entry.name, PATH_MAX);
			write(fdout, " ", 2);
		}
		root_len -= (entry.len + sizeof(entry));
		/* we don't need content */
		lseek(fd, entry.len, SEEK_CUR);
	}

	write(fdout, next_line, 3);
	lseek(fd, 0, SEEK_SET);
}

void show_cat(int argc, char *argv[]) {
	int fd = open("/dev/rom0", 0);
	struct romfs_entry entry;
	/* root */
	read(fd, &entry, sizeof(entry));
	
	int root_len = entry.len - sizeof(entry);
	while(root_len > 0) {

		/* FIXME
		 * Memmory access error (0x6XXXXXX)
		 * So cat not work
		 */
		read(fd, &entry, sizeof(entry));

		/* find file name */
		if(strcmp((char*)entry.name, argv[1]) == 0) {
			if(entry.isdir) {
					write(fdout, "It's a directory.", 18);
					write(fdout, next_line, 3);
					lseek(fd, 0, SEEK_SET);
					return;		
			}

			/* Print content.
			 * Single block r/w needs less than BLOCK_BUF
			 * see block.c
			 */
			while(entry.len) {
				char buf[BLOCK_BUF + 1];
				if(entry.len > BLOCK_BUF) {
					read(fd, buf, BLOCK_BUF);
					puts(buf);
					entry.len -= BLOCK_BUF;
				}
				else {
					read(fd, buf, entry.len);
					puts(buf);
					entry.len = 0;
					lseek(fd, 0, SEEK_SET);
					write(fdout, next_line, 3);
					return;		
				}
			}
			write(fdout, next_line, 3);
		}
		root_len -= (entry.len + sizeof(entry));
		lseek(fd, entry.len, SEEK_CUR);
	}
	write(fdout, "No file found.", 15);
	write(fdout, next_line, 3);
	lseek(fd, 0, SEEK_SET);
}

void itoa(int n, char *dst, int base)
{
	char buf[33] = {0};
	char *p = &buf[32];
	
	if (n == 0)
		*--p = '0';
	else {
		unsigned int num = (base == 10 && num < 0) ? -n : n;

		for (; num; num/=base)
			*--p = "0123456789ABCDEF" [num % base];
		if (base == 10 && n < 0)
			*--p = '-';
	}

	strcpy(dst, p);
}

int __attribute__((weak)) main() {

	pthread_t serialout_t, serialin_t, rs232_xmit_msg_task_t, serial_test_task_t;
	pthread_attr_t a, b, c, d;

	pthread_attr_init(&a);
	pthread_attr_init(&b);
	pthread_attr_init(&c);
	pthread_attr_init(&d);
	a.sched_param.sched_priority = 0;
	b.sched_param.sched_priority = 0;
	c.sched_param.sched_priority = 18;
	d.sched_param.sched_priority = 10;

	pthread_create(&serialout_t, &a, (void*)serialout, NULL);
	pthread_create(&serialin_t, &b, (void*)serialin, NULL);
	pthread_create(&rs232_xmit_msg_task_t, &c, (void*)rs232_xmit_msg_task, NULL);
	pthread_create(&serial_test_task_t, &d, (void*)serial_test_task, NULL);

	return 0;
}
