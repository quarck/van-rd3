	.file	"tx.S"
#include "config.h"

__SREG__ = 0x3f
__SP_H__ = 0x3e
__SP_L__ = 0x3d
__CCP__  = 0x34
__tmp_reg__ = 0
__zero_reg__ = 1

#define RXB_LOW  r24
#define RXB_HIGH r25
#define RXB_LEN r22
#define LOOP_CNT r16
#define tmp_reg1 r17
#define tmp_reg2 r18
#define max_retry r19
#define current_byte r20
#define result r21
#define	BIT_LOOP_CNT r15

#define TCNT0   0x32
#define PIND	0x10
#define PORTB	0x18
#define RXD_PIN 7
#define RXD_PORT PIND
#define TXD_PIN 0
#define TXD_PORT PORTB
	.text



/*
 	ldi r24,lo8(rxb)
	ldi r25,hi8(rxb)
	ldi r22,lo8(6)
	rcall tx_packet

 */
.global	tx_packet
	.type	tx_packet, @function
tx_packet:
/*
 Arguments:
	r24:r25 -- rxb buffer address
	r22: -- buffer data length
*/

	push 	r15
	push 	r16
	push 	r17
	push 	r18
	push 	r19
	push 	r20
	push 	r21
	push	r30
	push 	r31

	clr 	result
	ldi		max_retry, lo8(255) 

// wait for the line to be idle for at least 12-13 TS (EOF + IFS)
wait_line_idle:
	// set line to default recessive state
	sbi		TXD_PORT, TXD_PIN
	// setup sending parameters -- loop cnt, and data ptr
	mov		LOOP_CNT, r22
	movw	r30,r24

	in 		tmp_reg1, TCNT0 	// read timer value
	ldi		tmp_reg2, 13
	add		tmp_reg1, tmp_reg2	// will wait up to 13 TS from now
line_idle_check:
#ifdef SIMULATOR_DEBUG
	sbis	RXD_PORT, RXD_PIN   	// check, if it is zero
#else
	sbic	RXD_PORT, RXD_PIN   	// check, if it is zero
#endif
	rjmp 	line_ok			// if current line st == 1
	clr		tmp_reg1		// line is 0,  update timer, 
	out 	TCNT0, tmp_reg1		// 
to_next_line_idle:
	dec 	max_retry		
	brne	wait_line_idle
	rjmp	fn_return_fail_2	// than -- return "line busy"
line_ok:	
	in 		tmp_reg2, TCNT0 	// read current timer
	cp		tmp_reg1, tmp_reg2
	brne	line_idle_check		// if we are not reached 13 TS

	// so we got line to be idle for 13, ready to start transmission

	// send preamble first
	// format extra 'fake' byte for preamble, and increment real cnt, so send 
	// preamble as a part of message
	// preamble is 000 01 111 01, this encodes 0x0e byte with E-Man
	ldi		current_byte, lo8(0x0e)
	inc		LOOP_CNT
	rjmp 	next_byte_preloaded

	// transmit message body:

next_byte:
	ld 		current_byte, Z+ 	// 2 clks
next_byte_preloaded:
	ldi		tmp_reg2, lo8(10) 	//  1 clk; val= 8 bits plus 2 man bits
	rjmp	skip_delay_next_bit

next_bit:
	rjmp 1f 
1: 	nop
next_bit_skip_3clk:
	rjmp 1f
1:	rjmp 2f
2:	rjmp 3f
3:

skip_delay_next_bit:
	sbrc 	current_byte, 7		// 1 clk if no skip, 2 clk if skip
	rjmp 	aa7					// always 2 clk
	nop 
	rcall 	orbitary_transmit_0	// 95 clk
	rjmp 	bb7					// 2 clk
aa7: 
	rcall 	orbitary_transmit_1	// 95 clk
	nop
	nop
bb7:
	ldi 	tmp_reg1,3			// 1
cc7:
	dec 	tmp_reg1			// 1
	brne 	cc7					// 1 if false, 2 if true

	// so, the following checks are always takes 4+4+4 clks, not depending on the way, 
	// how the code is executed
	// (not including out of big look cheks)
	
	cpi		tmp_reg2, lo8(7)	// 1 clkk
	breq 	do_man_bit_high		// 2 if branched, 1 if not
	nop							// so, if not -- add one extra
	cpi		tmp_reg2, lo8(2)	// 1 clk
	breq	do_man_bit			// 2 clk if branched, 1 if not
	nop							// so, if not -- add 1 clk,

	lsl		current_byte 		// 1 clk
	dec		tmp_reg2			// 1 clk
	brne	next_bit			// 2 if branched, 1 if not

	// out of big loops checks:
	dec 	LOOP_CNT		// 1 clks
	breq 	outloop			// 1 clks if false
	nop
	rjmp 	next_byte		// 2 clks , and 2 clks - load instr 

	// 
do_man_bit_high:
	rjmp 1f		// we are here by the first cond, 
1:	rjmp 2f	// we have to eath 6 clks, before doing anything 
2: 	rjmp 3f
3: 
	subi	current_byte, lo8(128)	// 1 clk, similar to xor-ing high bit with 1
	dec		tmp_reg2		// 1 clk
	rjmp	next_bit_skip_3clk		// 2 clk

do_man_bit:
	subi	current_byte, lo8(128)	// 1 clk, similar to xor-ing high bit with 1
	dec		tmp_reg2		// 1 clk

	// for the last bit we should say '0', '0' seq
	cpi 	LOOP_CNT, lo8(1) // 1 clk
	brne	not_last     // 2 clk if branched, 1 if not
	clr		current_byte		// +1 clk if not branched, so anyway, these 3 instrs takes 3clk
not_last:
	rjmp	next_bit_skip_3clk		// 2 clk

outloop:
	// eat 13 clks here
	ldi		tmp_reg1, lo8(4)
1:	dec 	tmp_reg1
	brne	1b
	nop

	sbi		TXD_PORT, TXD_PIN

	ldi		tmp_reg1, lo8(29)
1:	dec 	tmp_reg1
	brne	1b

	sbis	RXD_PORT, RXD_PIN	// skip if no ack, 1 clk if ack, 2 clk if no
	rjmp 	has_ack1			// jump ack, 2 clk if ack, 0 if no ack 
	ori		result, lo8(128)	// 1 clk if no ack
has_ack1:

	ldi 	tmp_reg1, lo8(41)
1:	dec 	tmp_reg1
	brne	1b
	rjmp 	2f
2:

	sbic	RXD_PORT, RXD_PIN	// skip if ack, this is the last sampling, 
							 	// do not worry about timing...
	ori		result, lo8(64)

fn_return:
	mov 	r24,result // result
fn_return_result:	
	pop		r31
	pop		r30
	pop 	r21
	pop 	r20
	
	pop 	r19
	pop 	r18
	pop 	r17
	pop 	r16
	pop		r15

	ret

fn_return_fail_2:
	ldi 	r24, lo8(2)
	rjmp 	fn_return_result

fn_return_fail_1:
	ldi 	r24, lo8(1)
	rjmp	fn_return_result

 
// fn has to be called before 4 clks to real frame start, 
// so rcall eats 4 clks, and function starts from the changing line state
// after changing line state, fn waits for sampling point (70% after frame
// start, reads value, and if it is differs -- directy jumps to 
// wait frame_start, as we loose arbitration)
// 70% - 89 clks
// routine, including call and ret takes 95
// sample at 89
orbitary_transmit_0:
	cbi		TXD_PORT, TXD_PIN
	ldi		tmp_reg1, 21
orb_loop0:
	dec		tmp_reg1
	cpi		tmp_reg1, 0
	brne		orb_loop0
	
	sbic		RXD_PORT, RXD_PIN
	rjmp 		we_have_loose_arbitration	

 	ret	

// fn has to be called before 4 clks to real frame start, 
// so rcall eats 4 clks, and function starts from the changing line state
// after changing line state, fn waits for sampling point (70% after frame
// start, reads value, and if it is differs -- directy jumps to 
// wait frame_start, as we loose arbitration)
// 70% - 89 clks
// routine, including call and ret takes 95
// sample at 89
orbitary_transmit_1:
	sbi		TXD_PORT, TXD_PIN
	ldi		tmp_reg1, 21
orb_loop1:
	dec		tmp_reg1
	cpi		tmp_reg1, 0
	brne		orb_loop1	
	
	sbis		RXD_PORT, RXD_PIN
	rjmp 		we_have_loose_arbitration	
 	ret				

we_have_loose_arbitration:
	pop 	tmp_reg1	// remove return address from the stack
	pop	tmp_reg1	// which is 2 bytes long 
	rjmp	to_next_line_idle

	.size	tx_packet, .-tx_packet
