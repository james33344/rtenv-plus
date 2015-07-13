	.syntax unified
	.word current_tcb
	.type USART1_IRQHandler, %function
	.global USART1_IRQHandler
USART1_IRQHandler:
	push {lr}
	bl c_usart1_handler
	bl set_pendsv
	nop
	pop {lr}
	bx lr

	.type SysTick_Handler, %function
    .global SysTick_Handler
SysTick_Handler:
    push {lr}
	bl c_systick_handler
    bl set_pendsv
    nop 
    pop {lr}
    bx lr

	.type PendSV_Handler, %function
	.global PendSV_Handler
PendSV_Handler:
	cpsid i
push {lr}
bl trace_pendsv_switch_prev
pop {lr}
	mrs r0, psp

	ldr r3, =current_tcb
	ldr r2, [r3]
	ldr r1, [r2]	

	stmdb r0!, {r7}
	stmdb r0!, {r4-r11,ip}
	str r0, [r2]

	stmdb sp!, {r3, lr}
	bl context_switch
	ldmia sp!, {r3, lr}

	ldr r1, [r3]
	ldr r0, [r1]

	ldmia r0!, {r4-r11,ip}
	ldmia r0!, {r7}
	msr psp, r0
push {lr}
bl trace_pendsv_switch_now
pop {lr}
	cpsie i
	bx lr

	.type SVC_Handler, %function
	.global SVC_Handler
SVC_Handler:
#push {lr}
#bl trace_interrupt_in
#pop {lr}
	push {lr}
	
	mrs r0, psp
	ldr r3, =current_tcb
	ldr r2, [r3]
	ldr r1, [r2]	

	stmdb r0!, {r7}
	stmdb r0!, {r4-r11,ip}
	str r0, [r2]

	stmdb sp!, {r3, lr}
	bl syscall_handler
	ldmia sp!, {r3, lr}

	ldr r1, [r3]
	ldr r0, [r1]

	ldmia r0!, {r4-r11,ip}
	ldmia r0!, {r7}

	bl set_pendsv
#push {lr}
#bl trace_interrupt_out
#pop {lr}
	pop {lr}
	bx lr
	

	.type task_start, %function
	.global task_start
task_start:
	/* save kernel state */
	ldr r3, =current_tcb
	/* Use CurrentTCB to get the CurrentTCB address. */
	ldr r1, [r3]
	/* The first item in CurrentTCB is the task top of stack. */
	ldr r0, [r1]
	
	ldmia r0!, {r4-r11,lr}
	ldmia r0!, {r7}

	/* switch to process stack pointer */
	msr psp, r0
	mov r0, #3
	msr control, r0
	isb
	bx lr
