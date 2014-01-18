#include "config.h"

#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define VAN_SPEED 125000UL

#define TIMER_SCALE 8


// stream is encoded with e-manchester, 
// so every 4 bits are encoded in 5 ts
#define BIT_TS_WIDTH (F_CPU / VAN_SPEED / TIMER_SCALE )

//
#define MAX_LAG  (BIT_TS_WIDTH/2)
	
#define RX_BUFFER_SIZE 256*3 


void delayms(uint16_t millis) 
{
	while ( millis ) {
		_delay_ms(1);
		millis--;
	}
}


// Define USART baud rate
// modes for U2X=0
#if defined (DEVICE_REV_1)
#  define USART_UBBR_VALUE 8 /* 115.2k (-3.5% error).*/
#elif defined (DEVICE_REV_2)
#  define USART_UBBR_VALUE 1 /* 1Mbit (0.0% error) (2Mbit with U2X) */
#else 
#  error "Unknown device"
#endif


#ifdef DEVICE_REV_2

#define SEND_ACK_PORT 	PORTB
#define SEND_ACK_DDR 	DDRB
#define SEND_ACK_PIN 	1
#define WAIT_ACK_PORT	PINC
#define WAIT_ACK_PIN	2


uint8_t waiting_ack;
uint8_t sending_ack;
// init ack system
#define INIT_ACK() {\
			waiting_ack = 0; \
			sending_ack = 0; \
			SEND_ACK_DDR |= (uint8_t) (1<<SEND_ACK_PIN); \
			SEND_ACK_PORT &= (uint8_t) ~(1<<SEND_ACK_PIN); \
			WAIT_ACK_PORT &=(uint8_t) ~(1<<WAIT_ACK_PIN); \
		}

// report acknowledge on receiving byte
#define SEND_ACK() { \
			sending_ack ^= (1<<SEND_ACK_PIN); \
			if (sending_ack) \
				SEND_ACK_PORT |= (uint8_t)(1<<SEND_ACK_PIN); \
			else\
				SEND_ACK_PORT &= (uint8_t)~(1<<SEND_ACK_PIN);  \
		}

// wait acknowledge after sending byte
#define IS_GOT_ACK() ((WAIT_ACK_PORT & ((uint8_t)(1<<WAIT_ACK_PIN))) == waiting_ack)
#define SWITCH_TO_NEXT_ACK()  waiting_ack ^= (uint8_t) (1<<WAIT_ACK_PIN)
#define WAIT_ACK() { while (!IS_GOT_ACK()) ; SWITCH_TO_NEXT_ACK(); }


#else 

#define SEND_ACK()
#define WAIT_ACK()
#endif


void USART_vInit(void)
{
	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSRC = (uint8_t)((1<<URSEL) | (0<<USBS)|(1<<UCSZ1)|(1<<UCSZ0));
#ifdef DEVICE_REV_2
	UCSRA  |= (1<<U2X);
	INIT_ACK();
#endif
	// Set baud rate
	UBRRH = (uint8_t)(USART_UBBR_VALUE>>8);
	UBRRL = (uint8_t)USART_UBBR_VALUE;

	// Enable receiver and transmitter
	UCSRB = (uint8_t)((1<<RXEN)|(1<<TXEN));
}

void USART_vSendByte(uint8_t u8Data)
{
	// Wait if a byte is being transmitted
	WAIT_ACK(); // wait for last byte to be transmitted
	while((UCSRA&(1<<UDRE)) == 0);
	// Transmit data
	UDR = u8Data;
}

uint8_t USART_vReceiveByte()
{
	uint8_t r;
	// Wait until a byte has been received
	while((UCSRA&(1<<RXC)) == 0) ;
	// Return received data
	r = UDR;
	SEND_ACK(); // aknowledge receiving
	return r;
}

void USART_sendCrLf()
{
	USART_vSendByte('\r');
	USART_vSendByte('\n');
}


uint8_t USART_hasByte()
{
	return (UCSRA&(1<<RXC)) != 0;
}


/*
 * Leds
 */

inline void led_show_num(uint8_t num) 
{
#if defined DEVICE_REV_1
	PORTC = num & 63;
#elif defined DEVICE_REV_2
	PORTC = num & 0x3;
#else 
# error "unknown device"
#endif
}

inline void led_init()
{
#if defined DEVICE_REV_1
	DDRC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5);/* set PC0-PC5 to output */
#elif defined DEVICE_REV_2
	DDRC |= (1<<PC0) | (1<<PC1);/* set PC0-PC1 to output */
#else 
# error "unknown device"
#endif
	led_show_num(0xff);
#ifndef SIMULATOR_DEBUG
	delayms(300);
#endif
	led_show_num(0);
}

/*
   ||Name||        CRC-15 VAN ISO/11519–3||
   ||Width||       15||
   ||Poly||        [F9D]||
   ||Init||        7FFF||
   ||RefIn||       False||
   ||RefOut||      False||
   ||XorOut||      7FFF||
   ||Check||       6B39||
 */
#define CRC_POLY 0xf9d

register uint8_t crc_h asm("r5");
register uint8_t crc_l asm("r4");

#define CRC_VALID() ((crc_h == (0xcc ^ 0xff)) && (crc_l == (0x90^0xfe)))

#define crc_add_1() crc_h ^= 0x80


#define crc_proceed_bit() \
		{					\
			if (crc_h & 0x80) {		\
				crc_h ^= CRC_POLY >> 8;	\
				crc_l ^= CRC_POLY & 0xff;	\
			}				\
							\
			/* this logically shifts 16-bit value to left */ \
			/*  crc_h is r5  */		\
			/*  crc_l is r4  */		\
			__asm__ __volatile__ (		\
			"	clc\n"  	/* clear carry flag */			\
			"	rol r4\n"  	/* rotate through carry left */		\
			"	rol r5\n"	/* rotate throught carry left */	\
				);			\
		}


void crc_init()
{
	crc_h = 0xff;
	crc_l = 0xfe;
}
void crc_fin()
{
	crc_h ^= 0xff;
	crc_l ^= 0xfe; // lower bit will be always zero due to <<=1
}


void fill_packet_crc(const uint8_t *v, uint8_t len, uint8_t *crc_place) 
{
#define CRC_INIT 0x7fff
#define CRC_XOR_OUT 0x7fff
	uint16_t r = CRC_INIT;
	uint8_t i, b;

	for ( i = 0; i < len; i++ ) {
		r ^= ((uint16_t)v[i]) << 7;
		for ( b = 0; b < 8; b++ ) 
			r = (r & 0x4000) ? (r << 1) ^ CRC_POLY 
					 : (r << 1);
		r &= 0x7fff;
	}

	r ^= CRC_XOR_OUT;
	r <<= 1;
	crc_place[0] = r >> 8;
	crc_place[1] = r & 0xff;
}



void init_timer0() 
{
	// set timer init value
	TCNT0 = 0;
	// init with /8 prescale
	TCCR0 = (uint8_t) (0 << CS02) | (1 << CS01) | (0 << CS00);
}

inline uint8_t get_timer0() 
{
	return TCNT0;
}

inline void set_timer0(uint8_t value)
{
	TCNT0 = value;
}

inline void zero_timer0()
{
	TCNT0 = (uint8_t) 0;

	// ignore prescallers, so pay no attention to +/- error of 1/16 of TS 
//	SFIOR |= (1 << 0); // reset timers 0 and 1 prescallers
}

void init_timer1() 
{
	// set timer init value
	TCNT1 = 0;
	// init without prescale
//	TCCR1A = 0;
	TCCR1B = (0<<CS12) | (0<<CS11) | (1<<CS10); 
}

inline uint16_t get_timer1() 
{
	return TCNT1;
}


void init_VAN_pins()
{
	// PB0 is connected to TxD, so output
	DDRB = (uint8_t)( (1 << DDB0)); // set to output
	
	// PD7 is connected to RxD, so input
	DDRD = (uint8_t)(0 << DDD7);

}

void pullup_unsed_pins()
{
	// set pull-ups on un-used pins:

	// PC0-PC5 are used for leds in rev1, 
	// PC0/PC1 are leds in rev2, PC2 is WAIT_ACK, PC3/PC4 are JPs in rev2
	// PC6 is reset
	// 
	PORTC = 0;

	// PD7 is RxD VAN
	// PD0-PD1 - USART
	// unused: PD2,PD3,PD4,PD5,PD6
	PORTD = (1<<PD2) | (1<<PD3) | (1<<PD4) | (1<<PD5) | (1<<PD6);

	// PB0 is TxD VAN
	// PB6,PB7 is XLAT1/XLAT2
	// unused normally (some of ports are connected to programmer):
	// PB5, PB4, PB3, PB2, PB1
	//
	PORTB = (1<<PB1) | (1<<PB2) | (1<<PB3) | (1<<PB4) | (1<<PB5);
}

register uint8_t tmp_reg asm ("r3");// = 1;

// routine for accessing raw line leve;
#define get_rxd_value()  (PIND & ((uint8_t)(1<<PD7))) 


#define  wait_4clk() \
	__asm__ __volatile__ (\
			"	rjmp 1f\n" \
			"1:	rjmp 2f\n" \
			"2:	\n"  )

#define  wait_8clk() \
	__asm__ __volatile__ (\
			"	rjmp 1f\n" \
			"1:	rjmp 2f\n" \
			"2:	rjmp 3f\n" \
			"3:	rjmp 4f\n" \
			"4:	\n" )



#define SAMPLE_POINT 8
#define SYNC_POINT 12

#define wait_pre_sample_point() { while ((get_timer0() & 0xf) != SAMPLE_POINT-1) ; }

#define wait_sample_point() { while ((get_timer0() & 0xf) != SAMPLE_POINT) ; }

#define wait_bit_start() { while ((get_timer0() & 0xf) != 0)  ; }

#define wait_synced_on_falling_edge() { \
					tmp_reg = 0; \
					while ((get_timer0() & 0xf) != SYNC_POINT-1) { \
						if (!get_rxd_value()) { \
							tmp_reg = get_timer0(); \
							break; \
						} \
					} \
					if (tmp_reg) { \
						TCNT0 -= tmp_reg; \
						while ((get_timer0() & 0xf) != SAMPLE_POINT) \
							; \
					} /* Otherwice - we are already at the sample point */ \
				}

#define wait_synced_on_rising_edge() {  \
					tmp_reg = 0; \
					while ((get_timer0() & 0xf) != SYNC_POINT-1) { \
						if (get_rxd_value()) { \
							tmp_reg = get_timer0(); \
							break; \
						} \
					} \
					if (tmp_reg) { \
						TCNT0 -= tmp_reg; \
						while ((get_timer0() & 0xf) != SAMPLE_POINT) \
							; \
					} /* Otherwice - we are already at the sample point */ \
				}


#define sample_rxd_value_int() \
			{ \
				wait_pre_sample_point(); \
				wait_sample_point(); \
				get_rxd_value(); \
			}

#define sample_rxd_value() (sample_rxd_value_int())


inline void set_txd_value(uint8_t v)
{
	if (v)
		PORTB |= (uint8_t)(1<<PB0);
	else
		PORTB &= (uint8_t)(~(1<<PB0));
}


uint8_t rxb[RX_BUFFER_SIZE];
uint8_t *rxb_byte_ptr; //= 0;

register uint8_t rxb_cur_byte asm ("r2");// = 1;

//uint16_t send_timings[128];
//uint16_t *st_ptr;

//
// "UI" stubs
//


void show_string (uint8_t id1, uint8_t id2, uint8_t id3)
{
	if (!id1) return;
	USART_vSendByte(id1);
	if (!id2) return;
	USART_vSendByte(id2);
	if (!id3) return;
	USART_vSendByte(id3);
}

void show_string_crlf (uint8_t id1, uint8_t id2, uint8_t id3)
{
	if (!id1) return;
	USART_vSendByte(id1);
	if (!id2) return;
	USART_vSendByte(id2);
	if (!id3) return;
	USART_vSendByte(id3);
	USART_sendCrLf();
}

void send_menu_opt(uint8_t n, uint8_t id1, uint8_t id2, uint8_t id3)
{
	USART_vSendByte(n); 
	show_string(' ', '-', ' ');
	show_string(id1, id2, id3);
	USART_sendCrLf();
}



uint8_t vtoh(uint8_t v) 
{
	v &= 0xf;
	if (v < 10)
		return v + '0';
	return v + 'a'-10;
}
void show_hex(uint8_t v)
{
	USART_vSendByte(vtoh(v>>4));
	USART_vSendByte(vtoh(v&0xf));
}


// returns 1 when data is sent completely
void send_captured()
{
	uint8_t *send_ptr = &rxb[0];
	uint8_t ln;

	while (send_ptr != rxb_byte_ptr) {
		ln = *send_ptr; ++send_ptr;
		while (ln > 0) {
			if (send_ptr == rxb_byte_ptr)
				break;
			show_hex(*send_ptr);
			++send_ptr;
			--ln;
		}
		USART_sendCrLf();
	}
}

//void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
//{
//	/* Wait for completion of previous write */
//	while(EECR & (1<<EEWE))
//		;
//	/* Set up address and data registers */
//	EEAR = uiAddress;
//	EEDR = ucData;
//	/* Write logical one to EEMWE */
//	EECR |= (1<<EEMWE);
//	/* Start eeprom write by setting EEWE */
//	EECR |= (1<<EEWE);
//}
//
//
//unsigned char EEPROM_read(unsigned int uiAddress)
//{
//	/* Wait for completion of previous write */
//	while(EECR & (1<<EEWE))
//		;
//	/* Set up address register */
//	EEAR = uiAddress;
//	/* Start eeprom read by writing EERE */
//	EECR |= (1<<EERE);
//	/* Return data from data register */
//	return EEDR;
//}



// res & 0x0f:
// 0 - success
// 1 - failed general
// 2 - failed due to line busy for too long
// high 2 bits if succeded - ack bits
uint8_t tx_packet(const uint8_t *pkt, uint8_t len);



void tx_cmd(uint8_t *pkt, uint8_t len)
{
	uint8_t r;
	fill_packet_crc(pkt, len, pkt+len);
	len += 2;
	r = tx_packet(pkt, len);
	show_string_crlf((r & 0x80) ? '-' : 'A', (r & 0x40) ? '-' : 'A', (r & 0x0f) + '0');		
}


inline uint8_t check_reset() 
{
	return (USART_hasByte() && USART_vReceiveByte() == 'r');
}

#ifdef DEVICE_REV_1
#define VER "rev1"
#elif defined(DEVICE_REV_2)
#define VER "rev2"
#else
#error "unknown rev"
#endif

#define PROGRAM_MENU  "\r\n\r\n---------\r\n"\
		     	"VAN moninor(" VER ") menu:\r\n"\
			"d - dump VAN bus\r\n"\
			"2 - send 'switch to internal CD' cmd\r\n"\
			"3 - send 'switch to radio' cmd\r\n"\
			"4 - send 'switch to cd changer' cmd\r\n"\
			"5 - send 'switch to cd changer' cmd and jump to dumping mode instantly\r\n"\
			"6 - send ' 4EC: RA- 8700D316FFFF0102FF003F87 A ' cmd\r\n"\
			"7 - same as above, and jump to recv\r\n"\
			\
			"a - 1183  pause \n"\
			"s - 1181  play  \n"\
			"d - 1101  power off\n"\
			\
			"m - monitor line with timer\r\n"\
			"p - USART performance test (will flood with chars)\r\n"\
			"r - reset device\r\n\r\n"\
			"> "


/*
wow! we are homing on the target...! don't worry, we are in no hurry.  
 
 another useful sequence i forgot to post: 
 
 direct access to cd using 1-6 keys on the radio 
 
 8C4: WA- 8A2201 A 
 8EC: WA- 4101 A 
 8C4: WA- 96 A 
 4EC: RA- 8600D316FFFF0101FF003F86 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 8C4: WA- 8A2241 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 4EC: RA- 8600D316FFFF0101FF003F86 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 8C4: WA- 8A2202 A 
 8EC: WA- 4102 A 
 8C4: WA- 96 A 
 4EC: RA- 8700D316FFFF0102FF003F87 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 8C4: WA- 8A2242 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 4EC: RA- 8700D316FFFF0102FF003F87 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 5E4: WA- 201F A 
 9C4: WA- 0082 A 
 
 on port 8C4 we see 8A22 insteal of the usual 8AC2. 
 on 8EC, 41 is the change cd command and the payload 
 is the cd number. 
 
 a short list of command i;ve seen on 8EC: 
 
        41FF    next cd 
        41FE    prev cd 
        410X    direct cd selection 
        4106 
        31FF    ? next track 
        31FE    ? prev track 
        1183    mute/pause ? 
        1181    unmute/play ? 
        1184    fwd 
        1183    rev? 
        1185    ?

		http://www.forum-auto.com/les-clubs/section4/sujet174953-595.htm
*/

int main(void) 
{
	uint8_t i;
	uint8_t *s_ptr;

full_reset:
	// do some hardware initialization
	init_timer0();
//	init_timer1();
	init_VAN_pins();
	led_init();
	pullup_unsed_pins();
	USART_vInit();

#ifdef SIMULATOR_DEBUG
	rxb[0] = 0x8e; rxb[1] = 0x4c;
	rxb[2] = 0x12; rxb[3] = 0x02;
	rxb[4] = 0xc9; rxb[5] = 0x60;
	tx_packet(rxb, 1);
#endif



reset:
	// wait until user says to dump anything
	
	s_ptr = (uint8_t*)PSTR( PROGRAM_MENU );
	while( (i=pgm_read_byte(s_ptr++))!=0 ) {
	    	USART_vSendByte(i); 
	}
	i = USART_vReceiveByte(); USART_vSendByte(i); USART_sendCrLf();

	if (i == 'd') {
		goto init_rx;
	} else if (i == 'm') {
		goto monitor_bus;
	} else if (i == '2') { // switch to internal cd
		rxb[0] = 0x8d; rxb[1] = 0x4c; rxb[2] = 0x12; rxb[3] = 0x02;
		tx_cmd(rxb, 4);
	} else if (i == '3') { // switch to radio
		rxb[0] = 0x8d; rxb[1] = 0x4c; rxb[2] = 0x12; rxb[3] = 0x01;
		tx_cmd(rxb, 4);
	} else if (i == '4') { // switch to cdc
		rxb[0] = 0x8d; rxb[1] = 0x4c; rxb[2] = 0x12; rxb[3] = 0x03;
		tx_cmd(rxb, 4);
	} else if (i == '5') { // switch to cdc
		rxb[0] = 0x8d; rxb[1] = 0x4c; rxb[2] = 0x12; rxb[3] = 0x03;
		tx_cmd(rxb, 4);
		goto init_rx;
	} else if (i == 'a') { // pause cmd to cdc
		rxb[0] = 0x8e; rxb[1] = 0xcc; rxb[2] = 0x11; rxb[3] = 0x83;
		tx_cmd(rxb, 4);
	} else if (i == 's') { // play cmd to cdc
		rxb[0] = 0x8e; rxb[1] = 0xcc; rxb[2] = 0x11; rxb[3] = 0x81;
		tx_cmd(rxb, 4);
	} else if (i == 'd') { // turn off cmd to cdc
		rxb[0] = 0x8e; rxb[1] = 0xcc; rxb[2] = 0x11; rxb[3] = 0x01;
		tx_cmd(rxb, 4);
	} else if (i == '6' || i == '7') { // switch to cdc
		rxb[0] = 0x4e; 
		rxb[1] = 0xce; 
		rxb[2] = 0x87;
		rxb[3] = 0x00;
		rxb[4] = 0xD3;
		rxb[5] = 0x16;
		rxb[6] = 0xFF;
		rxb[7] = 0xFF;
		rxb[8] = 0x01;
		rxb[9] = 0x02;
		rxb[10] = 0xFF;
		rxb[11] = 0x00;
		rxb[12] = 0x3F;
		rxb[13] = 0x87;
		tx_cmd(rxb, 14);
			
		if (i == '7')
			goto init_rx;
	} else if (i == 'p') {
		for (i=0;; ) {
			USART_vSendByte('a'+i); USART_vSendByte('b'+i); USART_vSendByte('c'+i); 
			USART_vSendByte('d'+i); USART_vSendByte('e'+i); USART_vSendByte('f'+i);
			USART_vSendByte('g'+i); USART_vSendByte('h'+i);
			USART_sendCrLf(); 
			++i;
			i &= 0xf;
		}
	} else if (i == 'r') {
		goto full_reset;
	}

	goto reset;
init_rx:
	set_txd_value(1);

	// fill buffer until filled
	rxb_byte_ptr = &rxb[0];

	//
	// Wait for frame start:
	//
	// wait until anything is changed on the line,
	// busy line is 1, peamble is starting from zero.
	// so actually wait for preamble
	//
wait_frame_start: 
	// check for 'r' pressed
	if (check_reset()) {
		send_captured();
		goto reset;
	}

	// before the real frame is started line should be free for 8 TS, wait at least for 6 TS, 
	// so, if we have no seen logical '1' for 6 or more ts before this 'frame start', - this is 
	// noice or part of prev msg, ignoring...
	i = 0;
	zero_timer0();
	crc_init();

	while (get_rxd_value() != 0) { 
		if (i != 255) // check for overflow
			i = get_timer0();
	}

	// here we got if we got non-zero, reset timer first of all, and only after tmr reset -- 
	// check, how long line was in '1' state
	zero_timer0();

	if (i < 6) 
		goto wait_frame_start;
	

	// 
	// receive 10-bit with preamble
	// preable is a fixed format: 0000111101
	//
	
	// check first 4 bits to be all zeros
	if (sample_rxd_value() != 0) 
		goto wait_frame_start;
	if (sample_rxd_value() != 0) 
		goto wait_frame_start;
	if (sample_rxd_value() != 0) 
		goto wait_frame_start;
	if (sample_rxd_value() != 0) 
		goto wait_frame_start;

	// check next 4 bits to be all onces:
	wait_synced_on_rising_edge();
	if (get_rxd_value() == 0) 
		goto wait_frame_start;
	if (sample_rxd_value() == 0) 
		goto wait_frame_start;
	if (sample_rxd_value() == 0) 
		goto wait_frame_start;
	if (sample_rxd_value() == 0) 
		goto wait_frame_start;

	// check last 2 preamble bits to be 0/1
	wait_synced_on_falling_edge();
	if (get_rxd_value() != 0) 
		goto wait_frame_start;
	
	wait_synced_on_rising_edge();
	if (get_rxd_value() == 0) 
		goto wait_frame_start;

	//
	// receive message, it consists of:
	// * 12-bit ident and 4-bit com according to e-manchester, it takes, 15 ts-es + 4ts-es, = 20 ts
	// * up to 28 bytes of data
	// * 15 bits of ctrl terminated by 2TS eod, so in total - 20 ts (like 2 bytes)
	// So, max msg length is 32 bytes (not including acknowledge field)
	//
	// So, finally, read up to 2+28+2=32 bytes, untill reaches EOD
	//

	// reserve room for msg length
	s_ptr = rxb_byte_ptr;
	++rxb_byte_ptr;

//	wait_pre_sample_point(); 
//	wait_sample_point(); 

	for (i=0; i<32; i++) { // do not allow to overflow buffer
		rxb_cur_byte = 0;
		// decode higher 4-bits:
		// get NRZ bits

		if (sample_rxd_value()) { 
			rxb_cur_byte |= (uint8_t)0x80; 
			crc_add_1();
		}
		crc_proceed_bit();

		if (sample_rxd_value()) { 
			rxb_cur_byte |= (uint8_t)0x40;
			crc_add_1();
		}
		crc_proceed_bit();
			
		if (sample_rxd_value()) {
			rxb_cur_byte |= (uint8_t)0x20;
			crc_add_1();
		}
		crc_proceed_bit();

		// get MAN 1st TS
		if (sample_rxd_value()) {
			rxb_cur_byte |= (uint8_t)0x10;	
			crc_add_1();
			wait_synced_on_falling_edge();	
		} else {
			wait_synced_on_rising_edge();	
		}
		crc_proceed_bit();
		// EOD is not allowed here, so simply ignore second bit, 
		// two zeroes are possible here in case of arbitration procedure
		//we_do_not_need_this_bit = get_rxd_value();

		// lower 4-bits
		// get NRZ bits
		if (sample_rxd_value()) {
			rxb_cur_byte |= (uint8_t)0x08;
			crc_add_1();
		}
		crc_proceed_bit();
			
		if (sample_rxd_value()) {
			rxb_cur_byte |= (uint8_t)0x04;
			crc_add_1();
		}
		crc_proceed_bit();
		
		if (sample_rxd_value()) {
			rxb_cur_byte |= (uint8_t)0x02;
			crc_add_1();
		}
		crc_proceed_bit();

		// get MAN 1st TS
		if (sample_rxd_value()) {
			rxb_cur_byte |= (uint8_t)0x01;
			crc_add_1();
			// get MAN 2nd TS, sync on it
			wait_synced_on_falling_edge();
			if (get_rxd_value() != 0) 
				goto go_break; // not a EOD, but error
		} else {
			// get MAN 2nd TS and check for EOD marker
			wait_synced_on_rising_edge();
			if (get_rxd_value() == 0) 
				goto go_break;	
		}
		crc_proceed_bit();


		*rxb_byte_ptr = rxb_cur_byte;
		++ rxb_byte_ptr;
		continue;
go_break:
		crc_proceed_bit(); // not sure it is required here, may be we just need check crc here?
		*rxb_byte_ptr = rxb_cur_byte;
		++ rxb_byte_ptr;
				
		break;
	}

	// now, receive 2 last ACK bits, if any

	rxb_cur_byte = 0;

	if (sample_rxd_value()) 
		rxb_cur_byte |= (uint8_t)0x80;
	if (sample_rxd_value()) 
		rxb_cur_byte |= (uint8_t)0x40;
	
	// sample some more bits, for debugging
	if (sample_rxd_value()) 
		rxb_cur_byte |= (uint8_t)0x20;
	if (sample_rxd_value()) 
		rxb_cur_byte |= (uint8_t)0x10;
	if (sample_rxd_value()) 
		rxb_cur_byte |= (uint8_t)0x08;
	if (sample_rxd_value()) 
		rxb_cur_byte |= (uint8_t)0x04;

	// verify crc and add 'crc-valid' marker
	if (CRC_VALID())
		rxb_cur_byte |= (uint8_t) 0x02;
	else
		rxb_cur_byte |= (uint8_t) 0x01;

	*rxb_byte_ptr = rxb_cur_byte;
	// reset bit ptr for next msg, increase byte write ptr
	++ rxb_byte_ptr;

	*s_ptr = rxb_byte_ptr - s_ptr -1;

	if (rxb_byte_ptr-&rxb[0] > RX_BUFFER_SIZE - 50) {
		send_captured();
		rxb_byte_ptr = &rxb[0];
	}
	goto wait_frame_start;


monitor_bus:
	if (check_reset() ) 
		goto reset;

	rxb_byte_ptr = &rxb[0];
	zero_timer0();
	tmp_reg = get_rxd_value();
	*rxb_byte_ptr = tmp_reg; 
	++rxb_byte_ptr;
	while (rxb_byte_ptr != &rxb[RX_BUFFER_SIZE-1]) {
		while ((i = get_rxd_value()) == tmp_reg)  // wait for line to be changed 
			;
		*rxb_byte_ptr = get_timer0();
		++rxb_byte_ptr;
		tmp_reg = i;
		if (check_reset()) {
			for (s_ptr=&rxb[0]; s_ptr < rxb_byte_ptr;  ++ s_ptr ) {
					show_hex(*s_ptr);
			}
			USART_sendCrLf();

			goto reset;
		}
	}
	for (s_ptr=&rxb[0]; s_ptr < rxb_byte_ptr;  ++ s_ptr ) {
		show_hex(*s_ptr);
	}
	USART_sendCrLf();
	
	goto monitor_bus;

	return 0;
}

