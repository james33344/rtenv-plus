.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.global fork
fork:
    push {r7, lr}
    mov r7, #0x1
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global getpid
getpid:
    push {r7, lr}
    mov r7, #0x2
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global write
write:
    push {r7, lr}
    mov r7, #0x3
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global read
read:
    push {r7, lr}
    mov r7, #0x4
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global interrupt_wait
interrupt_wait:
    push {r7, lr}
    mov r7, #0x5
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global getpriority
getpriority:
    push {r7, lr}
    mov r7, #0x6
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global setpriority
setpriority:
    push {r7, lr}
    mov r7, #0x7
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global mknod
mknod:
    push {r7, lr}
    mov r7, #0x8
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global sleep
sleep:
    push {r7, lr}
    mov r7, #0x9
    svc 0
    nop
    pop {r7,lr}
    bx lr
.global lseek
lseek:
    push {r7, lr}
    mov r7, #0xa
    svc 0
    nop
    pop {r7,lr}
    bx lr
.global task_block
task_block:
    push {r7, lr}
    mov r7, #0xb
    svc 0
    nop
    pop {r7,lr}
    bx lr
.global mutex_lock
mutex_lock:
    push {r7, lr}
    mov r7, #0xc
    svc 0
    nop
    pop {r7, lr}
    bx lr
.global mutex_unlock
mutex_unlock:
    push {r7, lr}
    mov r7, #0xd
    svc 0
    nop
    pop {r7, lr}
    bx lr
