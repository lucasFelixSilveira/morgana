.data
    morgana_alert_msg_asciz: .asciz "Alert\r"
    morgana_alert_nine_plus: .asciz "[9+] "
.text
.type __morgana_alert, @function
.globl __morgana_alert
__morgana_alert:
    push %rbp
    movq %rsp, %rbp
    subq $16, %rsp

    cmpq $9, %r15
    ja .nine_plus

    movq %rsp, %rsi
    movb $'0', %al
    addb %r15b, %al
    movb $'[', (%rsi)
    movb %al, 1(%rsi)
    movb $']', 2(%rsi)
    movb $' ', 3(%rsi)
    movb $0, 4(%rsi)

    movq $1, %rax
    movq $1, %rdi
    movq $5, %rdx
    syscall
    jmp .done

.nine_plus:
    movq $1, %rax
    movq $1, %rdi
    movq $morgana_alert_nine_plus, %rsi
    movq $5, %rdx
    syscall

.done:
    movq $1, %rax
    movq $1, %rdi
    movq $morgana_alert_msg_asciz, %rsi
    movq $7, %rdx
    syscall
    leave
    ret

.text
.globl main
.type main, @function
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	sub $16, %rsp
	movl $73, -4(%rbp)
	movw $73, -6(%rbp)
	movl -4(%rbp), %eax
	movw -4(%rbp), %bx
	movzx %bx, %ebx
	cmpq %rax, %rbx
	jl .main_pular
	incq %r15
	call __morgana_alert

.main_pular:
	movq %rax, %rdi
.LFE0:
	.size main, .LFE0 - main
	movq %rdi, %rax
	leave
	ret
.text
.type _start, @function
.globl _start
_start:
    pushq %rbp
    movq %rsp, %rbp
    sub $16, %rsp
    call main
    movq %rax, %rcx

    movq $1, %rax
    movq $1, %rdi
    movq %rsp, %rsi
    movb $'\n', (%rsi)
    movb $0, 1(%rsi)
    movq $2, %rdx
    syscall

    movq %rcx, %rdi
    movq $60, %rax
    syscall

