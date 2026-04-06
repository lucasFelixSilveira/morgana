.globl _start
_start:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	movl $110, -4(%rbp)
.LFE0:
	.size _start, .LFE0 - _start
	movq %rdi, %rax
	leave
	ret
