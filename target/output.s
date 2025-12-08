.text
.globl _start
_start:
    movq %rsp, %rbp
    sub $2, %rsp
    movb $11, -1(%rbp)
    movb $24, -2(%rbp)
    mov %rbp, %rdi

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
	subq $2, %rsp
	movb -1(%rdi), %al
	movb %al, -1(%rbp)
	movb -2(%rdi), %al
	movb %al, -2(%rbp)
.LFE0:
	movq %rbp, %rsp
	popq %rbp
	ret
