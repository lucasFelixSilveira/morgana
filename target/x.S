.text
.globl _start
_start:
    call main
    movq %rax, %rdi   # exit code
    movq $60, %rax    # exit syscall
    syscall

.text
.globl main
.type main, @function
main:
    pushq %rbp
    movq %rsp, %rbp
    subq $16, %rsp    # aloca espaço na stack (alinhado)

    movb $72, -7(%rbp)  # H
    movb $69, -6(%rbp)  # E
    movb $76, -5(%rbp)  # L
    movb $76, -4(%rbp)  # L
    movb $79, -3(%rbp)  # O
    movb $10, -2(%rbp)  # \n
    movb $0,  -1(%rbp)  # \0

    leaq -7(%rbp), %rdi
    call morg.print

    xorq %rax, %rax     # return 0
    leave               # equivalente a movq %rbp,%rsp ; popq %rbp
    ret

.globl morg.print
.type morg.print, @function
morg.print:
    pushq %rbp
    movq %rsp, %rbp

    movq %rdi, %rsi     # %rsi = ponteiro do buffer (não mexer mais)
    movq %rdi, %rcx     # usar %rcx como ponteiro temporário para calcular length

    xorq %rdx, %rdx     # length = 0

.strlen_loop:
    cmpb $0, (%rcx)
    je .strlen_done
    incq %rdx
    incq %rcx
    jmp .strlen_loop

.strlen_done:
    movq $1, %rdi       # fd = stdout
    movq $1, %rax       # syscall write
    syscall

    leave
    ret
