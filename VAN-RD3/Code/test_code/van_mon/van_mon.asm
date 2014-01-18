
.device ATmega8

.def cnt0=r0 



.CSEG
; interrupt vectors
	
	rjmp 	start
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt
	rjmp 	bad_interrupt

bad_interrupt:
	rjmp bad_interrupt

start:
	ldi	r16,low(RAMEND)
	out 0x3d, r16
	ldi r16,high(RAMEND)
	out 0x3e, r16

wait_line_idle:
	ldi r24,low(rx_buf)
	ldi	r25,high(rx_buf)

	movw r30,r24

	ldi	r16,0x5c
	st	Z+,r16
	ldi	r16,0xe4
	st	Z+,r16
	ldi r16,0x05
	st 	Z+,r16	
	ldi r16,0x46
	st	Z+,r16
	ldi	r16,0x5c
	st	Z+,r16
	ldi	r16,0xe4
	st	Z+,r16
	ldi r16,0x05
	st 	Z+,r16	
	ldi r16,0x46
	st	Z+,r16

	
	ldi r22, 8

next_byte:
	ld r16, Z+ 		; 2 clks
	

	sbrc r16, 7					; 1 clk if no skip, 2 clk if skip
	rjmp aa7					; always 2 clk
	nop 
	rcall orbitary_transmit_0	; 95 clk
	rjmp bb7					; 2 clk
aa7: 
	rcall orbitary_transmit_0	; 95 clk
	nop
	nop
; so, cleared, timings:
; 2 (sbrc) + 1(nop) + 95 + 2 (rjmp)
; non-cleared, timings:
; 1 (sbrc) + + 2 (rjmp) + 95 + 2 (nop nop)

; so, 100 clks on a both ways, has 28 clks left to waste time
bb7:
	ldi r17,9		; 1
cc7:dec r17			; 1
	brne cc7		; 1 if false, 2 if true
	nop

	sbrc r16, 6
	rjmp aa6
	nop 
	rcall orbitary_transmit_0
	rjmp bb6				
aa6:rcall orbitary_transmit_0
	nop
	nop
bb6:ldi r17,9
cc6:dec r17	
	brne cc6
	nop


	sbrc r16, 5
	rjmp aa5
	nop 
	rcall orbitary_transmit_0
	rjmp bb5				
aa5:rcall orbitary_transmit_0
	nop
	nop
bb5:ldi r17,9
cc5:dec r17	
	brne cc5
	nop


	sbrc r16, 4
	rjmp aa4
	nop 
	rcall orbitary_transmit_0
	rjmp bb4				
aa4:rcall orbitary_transmit_0
	nop
	nop
bb4:ldi r17,9
cc4:dec r17	
	brne cc4
	nop

// we need E-MAN here, dude
	sbrs r16, 4
	rjmp am4
	nop 
	rcall orbitary_transmit_0
	rjmp bm4				
am4:rcall orbitary_transmit_0
	nop
	nop
bm4:ldi r17,9
cm4:dec r17	
	brne cm4
	nop


	sbrc r16, 3
	rjmp aa3
	nop 
	rcall orbitary_transmit_0
	rjmp bb3				
aa3:rcall orbitary_transmit_0
	nop
	nop
bb3:ldi r17,9
cc3:dec r17	
	brne cc3
	nop

	sbrc r16, 2
	rjmp aa2
	nop 
	rcall orbitary_transmit_0
	rjmp bb2				
aa2:rcall orbitary_transmit_0
	nop
	nop
bb2:ldi r17,9
cc2:dec r17	
	brne cc2
	nop

	sbrc r16, 1
	rjmp aa1
	nop 
	rcall orbitary_transmit_0
	rjmp bb1				
aa1:rcall orbitary_transmit_0
	nop
	nop
bb1:ldi r17,9
cc1:dec r17	
	brne cc1
	nop


	sbrc r16, 0
	rjmp aa0
	nop 
	rcall orbitary_transmit_0
	rjmp bb0				
aa0:rcall orbitary_transmit_0
	nop
	nop
bb0:ldi r17,9
cc0:dec r17	
	brne cc0
	nop


// we need to send ma bits here
	sbrs r16, 0
	rjmp am0
	nop 
	rcall orbitary_transmit_0
	rjmp bm0				
am0:rcall orbitary_transmit_0
	nop
	nop
bm0:ldi r17,7
cm0:dec r17	
	brne cm0
	nop


	dec r22				; 1 clks
	breq outloop		; 1 clks if false
	rjmp next_byte		; 2 clks , and 2 clks - load instr 
outloop:
	rjmp start

orbitary_transmit_0:
	cbi	24, 0
	ldi	r16, 21
orb_loop0:
	dec	r16
	cpi	r16, 0
	brne	orb_loop0	
	
	sbic	16, 7
	rjmp 	wait_line_idle		
 	ret				



.DSEG
rx_buf:	.BYTE 512
unused: .BYTE 510
RAMEND: .BYTE 2

