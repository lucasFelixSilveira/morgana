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
	movl $34, -5(%rbp)
	movb $0, -1(%rbp)
.LFE0:
	.size main, .LFE0 - main
	.cfi_endproc
	movq %rdi, %rax
	ret
