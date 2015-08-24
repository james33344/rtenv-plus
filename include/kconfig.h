#ifndef KCONFIG_H
#define KCONFIG_H

#define STACK_SIZE 384 /* Size of task stacks in words */
#define TASK_LIMIT 10  /* Max number of tasks we can handle */
#define TASK_BLOCK_LIMIT (TASK_LIMIT - 1) /* Max number of tasks whick can be block from other task */
#define PIPE_LIMIT (TASK_LIMIT * 2)
#define PIPE_BUF   64 /* Size of largest atomic pipe message */
#define PATH_MAX   32 /* Longest absolute path */
#define PATHSERVER_FD (TASK_LIMIT + 4) 
	/* File descriptor of pipe to pathserver */
#define FREG_LIMIT 16 /* Other types file limit */
#define FILE_LIMIT (PIPE_LIMIT + FREG_LIMIT)
#define MEM_LIMIT (2048)
#define BLOCK_BUF 64
#define REGFILE_BUF 64
#define FS_LIMIT 8
#define FS_TYPE_MAX 8
#define MOUNT_LIMIT 4

/*	PATHSERVER_FD + 1 to FILE_LIMIT is control by pathserver() 
 *	If we want to add new system FD:
 *	let PATHSERVER_FD += 1,
 *	and NEW_SYSTEM_FD = (PATHSERVER - N)
 *	
 *	OR
 *
 *	Use 0-2 reserved FDs
 *
 *	see SIGSERVER_FD
 */
#define SIGSERVER_FD (PATHSERVER_FD - 1)

#define ROMFS_FILE_LIMIT 8

#define INTR_LIMIT 58 /* IRQn = [-15 ... 42] */
#define EVENT_LIMIT (FILE_LIMIT * 2 + INTR_LIMIT + TASK_BLOCK_LIMIT + 1)
    /* Read and write event for each file, intr events and time event */

#define PRIORITY_DEFAULT 20
#define PRIORITY_LIMIT (PRIORITY_DEFAULT * 2 - 1)

#endif
