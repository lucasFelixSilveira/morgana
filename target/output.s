.text
.globl main
.type main, @function
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	sub $32, %rsp
	movq $0, %rax
	movq $15, -12(%rbp)
	movq $12, %rax
	addq $3, %rax
	movq %rax, -20(%rbp)
	movl -20(%rbp), %eax
	movl %eax, -4(%rbp)
.LFE0:
	.size main, .LFE0 - main
	movq %rdi, %rax
	leave
	ret
.text
.type _start, @function
.globl _start
_start:
    pushq %rbp
    movq %rsp, %rbp
    sub $16, %rsp
    call main

    movq %rax, %rdi
    movq $60, %rax
    syscall

