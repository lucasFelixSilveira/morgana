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
	movb $72, -14(%rbp)
	movb $69, -13(%rbp)
	movb $76, -12(%rbp)
	movb $76, -11(%rbp)
	movb $79, -10(%rbp)
	movb $32, -9(%rbp)
	movb $87, -8(%rbp)
	movb $79, -7(%rbp)
	movb $82, -6(%rbp)
	movb $76, -5(%rbp)
	movb $68, -4(%rbp)
	movb $33, -3(%rbp)
	movb $10, -2(%rbp)
	movb $0, -1(%rbp)
	leaq -14(%rbp), %rdi
	call morg.print
.LFE0:
	movq %rbp, %rsp
	popq %rbp
	ret

.text
.type morg.print, @function
.globl morg.print
morg.print:
    movq    %rdi, %rsi
    movq    $1, %rax
    movq    $1, %rdi
    call    morg.print.strlen
    syscall
    ret

morg.print.strlen:
    movq    %rcx, %r13
    xorq    %rcx, %rcx

morg.print.strlen.loop:
    cmpb    $0, (%rsi, %rcx)
    je      morg.print.strlen.end
    incq    %rcx
    jmp     morg.print.strlen.loop

morg.print.strlen.end:
    movq    %rcx, %rdx
    movq    %r13, %rcx
    xorq    %r13, %r13
    ret
