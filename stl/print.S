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
