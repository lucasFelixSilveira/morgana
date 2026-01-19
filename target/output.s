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
	sub $16, %rsp
	movb $72, -5(%rbp)
	movb $69, -6(%rbp)
	movb $76, -7(%rbp)
	movb $76, -8(%rbp)
	movb $79, -9(%rbp)
	movb $32, -10(%rbp)
	movb $87, -11(%rbp)
	movb $79, -12(%rbp)
	movb $82, -13(%rbp)
	movb $76, -14(%rbp)
	movb $68, -15(%rbp)
	movb $33, -16(%rbp)
	movb $10, -17(%rbp)
	movb $0, -18(%rbp)
	movb -5(%rbp), %dil
	movq %rdi, %rax
	jmp .LFE0

.LFE0:
	mov %rbp, %rsp
	pop %rbp
	ret
