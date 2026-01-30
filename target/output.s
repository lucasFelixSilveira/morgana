; gpio pin 2 placed in p2
.section .text
.global main
main:
	sbi 0x0A, 2
	sbi 0x0B, 2
