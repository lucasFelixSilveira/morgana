.text
.type _start, @function
.globl _start
_start:
    pushq %rbp
    movq %rsp, %rbp
    call main
    movl %eax, %edi
    movl $60, %eax
    syscall
    leave
.text
.globl main
.type main, @function
main:
.LFP0:
	.cfi_startproc
	movl $30, -5(%rbp)
	movb $4, -1(%rbp)
	movl -5(%rbp), %eax
	subb -1(%rbp), %ah
	movzx %ah, %eax
	movq %rax, -13(%rbp)
	movq -13(%rbp), %rax
	addq $3, %rax
	movq %rax, -21(%rbp)
.LFE0:
	.size main, .LFE0 - main
	.cfi_endproc
	movq %rdi, %rax
	ret
