.global morgana_delay_ms
morgana_delay_ms:
    movw r30, r24
    or r30, r31
    breq morgana_delay_end

morgana_delay_loop:
    ldi r18, 200
1:  ldi r19, 10
2:  nop
    dec r19
    brne 2b
    dec r18
    brne 1b
    sbiw r24, 1
    brne morgana_delay_loop
morgana_delay_end:
    ret

.section .text
.global main
main:
.LOOP0:
	cbi 0x0A, 3
	sbi 0x0B, 3
	in r24, 0x09
	andi r24, 8
	cpi r24, 0
	brne .main_led
	rjmp .main_zero
.main_um:
	sbi 0x0A, 2
	sbi 0x0B, 2
	rjmp .main_continuar
.main_zero:
	cbi 0x0B, 2
.main_continuar:
	rjmp .LOOP0
