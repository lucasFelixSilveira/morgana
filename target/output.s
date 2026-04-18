.globl main
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	movb $255, -1(%rbp)
.LFE0:
	.size main, .LFE0 - main
	movq %rdi, %rax
	leave
	ret
