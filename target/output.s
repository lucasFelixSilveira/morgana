.globl main
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	jmp .LFE0
.LFE0:
	.size main, .LFE0 - main
	movq %rdi, %rax
	leave
	ret
