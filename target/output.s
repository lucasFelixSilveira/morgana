.text
.globl _start
_start:
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
	subq $16, %rsp
	movl $24, -12(%rbp)
	movl $72, -8(%rbp)
	movl $36, -4(%rbp)
	leaq -12+8(%rbp), %rsi
	movl (%rsi), %edi
	movq %rdi, %rax
	jmp .LFE0
.LFE0:
	movq %rbp, %rsp
	popq %rbp
	ret
