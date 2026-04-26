.text
.globl WinMain
WinMain:
	call main
	ret
	
.text
.globl main
main:
.LFP0:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
	lea .fn0._1(%rip), %rcx
	movq %rcx, -8(%rbp)
	movq -8(%rbp), %rcx
	subq $32, %rsp
	call println
	addq $32, %rsp
.LFE0:
	leave
	ret
.data
	.fn0._1: .asciz "Hello, world! My name is Lucas! bruh"
