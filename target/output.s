.globl _start
_start:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	subq $8, %rsp
	movl $12, -4(%rbp)
	movl -4(%rbp), %eax
	movw %ax, -6(%rbp)

	movq %rax, %rdi
	movq $60, %rax
	syscall
.LFE0:
	.size _start, .LFE0 - _start
	movq %rdi, %rax
	leave
	ret
