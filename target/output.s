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
	lea .fn0.text(%rip), %rcx
	call printf
.LFE0:
	leave
	ret
.data
	.fn0.text: .asciz "Hello, World!"
