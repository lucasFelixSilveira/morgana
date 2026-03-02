.text
.globl main
.type main, @function
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	sub $48, %rsp
	movq $0, %rax
	movq $246, -12(%rbp)
	movq $82, %rax
	imulq $3, %rax
	movq %rax, -20(%rbp)
	movq $0, %rax
	movq $44, -28(%rbp)
	movq $12, %rax
	addq $32, %rax
	movq %rax, -36(%rbp)
	movq $0, %rax
	movq -36(%rbp), %rax
	addq -20(%rbp), %rax
	movq %rax, -44(%rbp)
	movl -44(%rbp), %eax
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

