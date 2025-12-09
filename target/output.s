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
	subq $6, %rsp
	movl %edi, -4(%rbp)
	movb -1(%rsi), %al
	movb %al, -1(%rbp)
	movb -2(%rsi), %al
	movb %al, -2(%rbp)
.LFE0:
	movq %rbp, %rsp
	popq %rbp
	ret
