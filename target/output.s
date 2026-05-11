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
	movq $34, -4(%rbp)
	movq -4(%rbp), %rdi
	jmp .LFE0
.LFE0:
	.size main, .LFE0 - main
	movq %rdi, %rax
	leave
	ret
.data
