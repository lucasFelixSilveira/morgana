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
	sbi 0x0A, 2
	sbi 0x0B, 2
	sbi 0x0A, 3
	cbi 0x0B, 3
	ldi r24, 0xF4
	ldi r25, 0x01
	rcall morgana_delay_ms
	cbi 0x0B, 2
	sbi 0x0B, 3
	ldi r24, 0xF4
	ldi r25, 0x01
	rcall morgana_delay_ms
	cbi 0x0B, 2
	cbi 0x0B, 3
	cbi 0x0A, 4
	sbi 0x0B, 4
	in r24, 0x09
	andi r24, 16
.LOOP0:
	in r24, 0x09
	andi r24, 16
	cpi r24, 0
	brne .main_high
	rjmp .main_low
.main_high:
	cbi 0x0B, 3
	sbi 0x0B, 2
	ldi r24, 0x2C
	ldi r25, 0x01
	rcall morgana_delay_ms
	cbi 0x0B, 2
	ldi r24, 0x2C
	ldi r25, 0x01
	rcall morgana_delay_ms
	cbi 0x0B, 3
	sbi 0x0B, 2
	rjmp .main_continue
.main_low:
	cbi 0x0B, 2
	sbi 0x0B, 3
	ldi r24, 0x2C
	ldi r25, 0x01
	rcall morgana_delay_ms
	cbi 0x0B, 3
	ldi r24, 0x2C
	ldi r25, 0x01
	rcall morgana_delay_ms
	cbi 0x0B, 2
	sbi 0x0B, 3
.main_continue:
	rjmp .LOOP0
