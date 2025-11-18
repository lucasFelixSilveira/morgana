.text
.globl _start
_start:
	call main
	movq %rax, %rdi
	movq $60, %rax
	syscall

.text
.globl main
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	subq $8, %rsp
	movb %dil, -5(%rbp)
	movl %esi, -4(%rbp)
.LFE0:
	movq %rbp, %rsp
	popq %rbp
	ret
