.text
.globl _start
_start:
	call main
	mov %rax, %rdi
	mov $60, %rax
	syscall

.text
.global main
.type main, @function
main:
.LFP0:
	push %rbp
	mov %rsp, %rbp
	sub $16, %rsp
	movl %edi, -4(%rbp)
	movl $82, -4(%rbp)
	movl -4(%rbp), %edi
	movq %rdi, %rax
	jmp .LFE0

.LFE0:
	mov %rbp, %rsp
	pop %rbp
	ret
