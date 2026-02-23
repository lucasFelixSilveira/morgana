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
.text
.globl main
.type main, @function
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	sub $16, %rsp
	movl $0, -4(%rbp)
	movq $0, %rax
	movl -4(%rbp), %eax
	addq $67, %rax
	movq %rax, -12(%rbp)
	movq %rax, %rdi
.LFE0:
	.size main, .LFE0 - main
	movq %rdi, %rax
	leave
	ret
