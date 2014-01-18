#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <util/delay.h>

#define VAN_SPEED 125000UL


// stream is encoded with e-manchester, 
// so every 4 bits are encoded in 5 ts
#define BIT_TS_WIDTH (F_CPU / VAN_SPEED )

//
#define MAX_LAG  (BIT_TS_WIDTH/2)
	
#define RX_BUFFER_SIZE 255 /*so, if rxb_ptr equals to 255, this means buffer overflowed*/


void delayms(uint16_t millis) 
{
	while ( millis ) {
		_delay_ms(1);
		millis--;
	}
}


// Define USART baud rate
// modes for U2X=0
#define USART_UBBR_VALUE 8 /* 115.2k (-3.5% error).*/
//#define USART_UBBR_VALUE 3 /* 250k (0% error)*/
//#define USART_UBBR_VALUE 1 /* 500k (0% error)*/
//#define USART_UBBR_VALUE 25 /* 38.4, 0.2% */

void USART_vInit(void)
{
// When the function writes to the UCSRC Register, the URSEL bit (MSB) must be set due 
// to the sharing of I/O location by UBRRH and UCSRC.

	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSRC = (uint8_t)((1<<URSEL) | (0<<USBS)|(1<<UCSZ1)|(1<<UCSZ0));

	// Set baud rate
	UBRRH = (uint8_t)(USART_UBBR_VALUE>>8);
	UBRRL = (uint8_t)USART_UBBR_VALUE;

	// UMSEL=0, means async mode (UART)
	// U2X=0, means no double speed async
	
	// UCSZ2 (UCSRB)
	// UCSZ1 (UCSRC)
	// UCSZ0 (UCSRC)
	// these are 3 bits controlling frame size:
	// 	0 0 0 - 5 bit
	//	0 0 1 - 6 bit
	//	0 1 0 - 7 bit
	// 	0 1 1 - 8 bit
	// 	1 0 0 - reserved
	// 	1 0 1 - reserved
	//	1 1 0 - reserved
	//	1 1 1 - 9 bit

	// UPM1, UPM0 (UCSRC)
	// these are 2 bits controlling framing
	// 	0 0 -- disabled
	// 	0 1 -- Reserved
	//	1 0 -- Even
	//	1 1 -- Odd

	// USBS (UCSRC) - stop bits:
	// 0 - 1 stop bit
	// 1 - 2 stop bits

	// Enable receiver and transmitter
	UCSRB = (uint8_t)((1<<RXEN)|(1<<TXEN));
}

/*
USART transmitter is enabled by setting TXEN bit in UCSRB register. Normal port operations is overridden by USART (XCK pin in synchronous operations will be overridden too).
*/
void USART_vSendByte(uint8_t u8Data)
{
	// Wait if a byte is being transmitted
	while((UCSRA&(1<<UDRE)) == 0);
	// Transmit data
	UDR = u8Data;
}

/*
The USART Receiver is enabled by writing the Receive Enable (RXEN) bit in the UCSRB Register to one. When the Receiver is enabled, the normal pin operation of the RxD pin is overridden by the USART and given the function as the Receiver’s serial input. The baud rate, mode of operation and frame format must be set up before any transfer operations.

*/
uint8_t USART_vReceiveByte()
{
	/*
	   The USART Receiver has three error flags: Frame Error (FE), Data OverRun (DOR) and Parity
	   Error (PE). All can be accessed by reading UCSRA. Common for the error flags is that they are
	   located in the receive buffer together with the frame for which they indicate the error status. Due
	   to the buffering of the error flags, the UCSRA must be read before the receive buffer (UDR),
	   since reading the UDR I/O location changes the buffer read location. Another equality for the
	   error flags is that they can not be altered by software doing a write to the flag location. However,
	   all flags must be set to zero when the UCSRA is written for upward compatibility of future
	   USART implementations. None of the error flags can generate interrupts.
	 */

	// Wait until a byte has been received
	while((UCSRA&(1<<RXC)) == 0) ;
	// Return received data
	return UDR;
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

#define LED_ON(x) PORTC &= ~(1<<x)
#define LED_OFF(x) PORTC |= (1<<x)

inline void led_show_num(uint8_t num) 
{
	uint8_t i,v;
	if (num > 63)
		num = 63; // absolute maximum for 6 leds
	for (i=0,v=1; i<<6; i++,v<<=1) {
		if (num & v)
			PORTC &= ~v;
		else
			PORTC |= v;
	}
}

inline void led_init()
{
	DDRC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5);/* set PC0-PC5 to output */
	led_show_num(0xff);
	delayms(300);
	led_show_num(0);
}

/*
  petit pas en avant: 
  il semblerait que l'algo du CRC pour le VAN soit: 
  x^14 + x^13 + x^12 + x^7 + x^6 + x^5 + x^4 + x^3 + 1
 */
#define POLY 0xf9d

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
uint16_t crc15_a(const unsigned char *v, int len) 
{
	uint16_t r = 0;
	int i, b;
	
	for ( i = 0; i < len; i++ ) {
		r |= v[i];
		for ( b = 0; b < 8; b++ ) 
			r = (r & 0x4000) ? (r << 1) ^ POLY
					 : (r << 1);
		r &= 0x7fff;
	}

	return r;
}


uint8_t tmr_prescale = 1; // default is not prescaled
			  // values are: 
			  // 1 - no presc
			  // 2 - /8
			  // 3 - /64
			  // 4 - /256
			  // 5 - /1024

void init_timer0() 
{
	// set timer init value
	TCNT0 = 0;

	switch (tmr_prescale) {
	default:
	case 1:
		// init timer, set timer pre-scaler to 'not used'
		TCCR0 = (uint8_t) (1 << CS00);
		tmr_prescale = 1;
		break;
	case 2:
		TCCR0 = (uint8_t) (0 << CS02) | (1 << CS01) | (0 << CS00);
		break;
	case 3:
		TCCR0 = (uint8_t) (0 << CS02) | (1 << CS01) | (1 << CS00);
		break;
	case 4:
		TCCR0 = (uint8_t) (1 << CS02) | (0 << CS01) | (0 << CS00);
		break;
	case 5:
		TCCR0 = (uint8_t) (1 << CS02) | (0 << CS01) | (1 << CS00);
		break;
	}
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
	SFIOR |= (1 << 0); // reset timers 0 and 1 prescallers
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

	// PC0-PC5 are used for leds
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

uint8_t rxd_invert = 0x0;

inline uint8_t get_rxd_value() 
{
	return (PIND & (1<<PD7)) ^ rxd_invert;
}

uint8_t txd_toggle = 0xff;

inline void set_txd_value(uint8_t v)
{
	if (txd_toggle) {
		if (v)
			PORTB |= (uint8_t)(1<<PB0);
		else
			PORTB &= (uint8_t)(~(1<<PB0));
	} else {
		if (v) {
			// set as output, ctl will pull-down line, resulting logical 0 on PB0  
			PORTB = 0;
			DDRB = (1<<PB0);
		} else {
			// set as input, and pull up, this will result logical high on PB0,  
			DDRB = ~(1<<PB0);
			PORTB = (1<<PB0);
		}
	}
}

uint8_t last_rxd_state = 0;
uint8_t receiving_frame = 0;

uint8_t rxb[RX_BUFFER_SIZE];

uint8_t rxb_cur_bit = 1;
uint8_t rxb_byte_ptr = 0;

void rxb_put_current_bit()
{
	if (~rxb_byte_ptr == 0)// will work only for 255-bytes sized buffer >= RX_BUFFER_SIZE)
		return ; // buffer overflow

	if (last_rxd_state) 
		rxb[rxb_byte_ptr] |= (uint8_t)rxb_cur_bit;
	
	rxb_cur_bit <<= 1;

	if (rxb_cur_bit == 0) {
		rxb_cur_bit = 1;
		rxb_byte_ptr ++; 
	}
}

void rxb_reset()
{
	rxb_cur_bit = 1;
	rxb_byte_ptr = 0;
}


// when compliled with -O9: 
// if the current line is '1' 
// this call takes normally 31 clk, or 36 clk if on the byte boundary,
// if the current line is '0', 
// this call takes normally  4 clk, or 9 clk if on the byte boundary

void sample_bit()
{
	rxb_put_current_bit();

	if (rxb_byte_ptr > 250)
		receiving_frame = 0; 
}

uint8_t vtoh(uint8_t v) 
{
	v &= 0xf;
	if (v < 10)
		return v + '0';
	return v + 'a'-10;
}



void recv_error()
{
	for (;;) {
		led_show_num(0xff);
		delayms(1000);
		led_show_num(0);
		delayms(1000);
	}
}

void show_string (uint8_t id1, uint8_t id2, uint8_t id3, uint8_t id4, uint8_t id5, uint8_t id6)
{
	if (!id1) return;
	USART_vSendByte(id1);
	if (!id2) return;
	USART_vSendByte(id2);
	if (!id3) return;
	USART_vSendByte(id3);
	if (!id4) return;
	USART_vSendByte(id4);
	if (!id5) return;
	USART_vSendByte(id5);
	if (!id6) return;
	USART_vSendByte(id6);
}
void show_hex(uint8_t v)
{
	USART_vSendByte(vtoh(v>>4));
	USART_vSendByte(vtoh(v&0xf));
}


void show_st(uint8_t v, uint8_t id1, uint8_t id2, uint8_t id3, uint8_t id4, uint8_t id5, uint8_t id6)
{
	show_string(id1, id2, id3, id4, id5, id6);
	show_string(' ', ':', ' ', 0, 0, 0);
	show_hex(v);
	USART_sendCrLf();
}

void send_menu_opt(uint8_t n, uint8_t id1, uint8_t id2, uint8_t id3, uint8_t id4, uint8_t id5, uint8_t id6)
{
	USART_vSendByte(n); 
	show_string(' ', '-', ' ', 0, 0, 0);
	show_string(id1, id2, id3, id4, id5, id6);
	USART_sendCrLf();
}

void debug_send_received_buf()
{
	uint8_t sptr;
	for (sptr = 0; sptr < rxb_byte_ptr; sptr ++) 
		show_hex(rxb[sptr]);
	USART_sendCrLf();
}


void sbclk(uint8_t dif) 
{
	show_string('s', 'a', 'm', 'p', 'l', 'e');
	show_string('_', 'b', 'i', 't', ' ', 't');
	show_string('a', 'k', 'e', 's', ' ', 'x');
	show_hex(dif);
	show_string(' ', 'c', 'l', 'k', ',', ' ');
}

int main(void) 
{
	uint8_t i;
	uint8_t txd_val = 0;

reset:
	init_timer0();

	init_VAN_pins();
	led_init();

	pullup_unsed_pins();
	
	USART_vInit();

	// count each BIT_TS_WIDTH-clock length intervals, starting at some random point...
menu:	
	for (i=0; i<3; i++) 
		USART_sendCrLf();
	for (i=0; i<20; i++) 
		USART_vSendByte('-');
	USART_sendCrLf();

	show_st(rxd_invert, 'r', 'x', 'd', 'i', 'n', 'v');
	show_st(txd_val, 't', 'x', 'd', 'v', 'a', 'l');
	show_st(txd_toggle, 't', 'x', 'd', 't', 'o', 'g');
	
	show_string('t', 'm', 'r', 'p', 'r', 'e'); show_string('s', 'c', 'a', 'l', 'e', ' ');
	show_string(':', ' ', 0, 0, 0, 0);
	switch (tmr_prescale) {
		case 1: show_string('/', '1', 0,  0, 0, 0); break;
		case 2: show_string('/', '8', 0, 0, 0, 0); break;
		case 3: show_string('/', '6', '4', 0, 0, 0); break;
		case 4: show_string('/', '2', '5', '6', 0, 0); break;
		case 5: show_string('/', '1', '0', '2', '4', 0); break;
	}
	USART_sendCrLf();

	
	USART_sendCrLf();

	send_menu_opt('1', 'd', 'e', 'c', 'o', 'd', 'e');
	send_menu_opt('x', 'X', 'X', 'c', 'o', 'd', 'e');
	send_menu_opt('2', 'c', 'o', 'l', 'l', 'c', 't');
	send_menu_opt('2', 'c', 'o', 'l', 'l', 'c', 't');
	send_menu_opt('3', 'm', 'o', 'n', 'i', 't', 'r');
	send_menu_opt('4', 'r', 'm', 'n', 'i', 't', 'r');
	USART_sendCrLf();
	
	send_menu_opt('q', 't', 'g', 'r', 'x', 'i', 'n');
	send_menu_opt('w', 't', 'g', 't', 'x', 'v', 'l');
	send_menu_opt('e', 't', 'g', 't', 'x', 't', 'g');
	USART_sendCrLf();

	send_menu_opt('y', '/', '1', ' ', 's', 'c', 0);
	send_menu_opt('u', '/', '8', 0,  0, 0, 0);
	send_menu_opt('i', '/', '6', '4', 0, 0, 0);
	send_menu_opt('o', '/', '2', '5', '6', 0, 0);
	send_menu_opt('p', '/', '1', '0', '2', '4', 0);
	USART_sendCrLf();

	send_menu_opt('s', 't', 's', 't', 'b', 't', 's');
	USART_sendCrLf();

	send_menu_opt('b', 'b', 'r', 'e', 'a', 'k', ' ');
	send_menu_opt('r', 'r', 'e', 's', 'e', 't', ' ');

	USART_vSendByte('>'); 
	USART_vSendByte(' '); 
	i = USART_vReceiveByte();
	USART_vSendByte(i);
	USART_sendCrLf();


#define CHECK_RST_BKR() if (USART_hasByte()) { uint8_t v = USART_vReceiveByte();  if (v == 'r') {debug_send_received_buf(); goto reset; } if (v == 'b') {debug_send_received_buf(); goto menu;}}

	switch (i)
	{
	case '1':
		set_txd_value(txd_val);
		zero_timer0();
		for(;;) {
			uint8_t cv;
			
			rxb_reset();

			// wait untill anything is changed on the line,
			// busy line is 1, peamble is starting from zero.
			// so actually wait for preamble
wait_preamble:
			while ((last_rxd_state=get_rxd_value()) != 0) {
				; // do nothing
				CHECK_RST_BKR();
			}
			if (get_rxd_value() != 0)   // avoid fluctuations
				goto wait_preamble; 

			// update state
			receiving_frame = 1;

			// set first sampling point 
			set_timer0(BIT_TS_WIDTH/2); // so, rise sample_bit after BIT_TS_WIDTH/2 cycles

			// and now, timer interrupt is enabled and scheduled, the following schedules will
			// be programmed by timer interrupt handler, here we only need to watch for 

			while (receiving_frame) {
				// if it's time to check value -- check value, 
				// and re-schedule the timer
				uint8_t tmr = get_timer0();
				if (tmr >= BIT_TS_WIDTH) {
					// code from get_timer0() to set_timer0(...) 
					// is tipically assembled to:
					//	in r25,82-32		# START,1clk
					//	cpi r25,lo8(100)	# 	1clk 
					//	brlo .L86		# 	1/2clk
					//	mov r24,r15		# 	1clk
					//	sub r24,r25		#	1clk
					//	out 82-32,r24		# END,	1clk
					//
					//  So, our calculation lag is 6-7 cpu clocks
					set_timer0(BIT_TS_WIDTH-6 - tmr); // this is more accurate than set_timer0
					if (tmr > BIT_TS_WIDTH + MAX_LAG)
						recv_error();					
					last_rxd_state = get_rxd_value(); 
					sample_bit();
					continue;
				}

				// if we are here, sample point is not reached, so carefully look 
				// for changed front, changed front is the only change to update
				// sync cnt
				cv = get_rxd_value(); 
				if (cv != last_rxd_state)  { 
					if (cv == get_rxd_value())  // to avoid fluctuations on line
						set_timer0(BIT_TS_WIDTH/2-4); // so, rise sample_bit after BIT_TS_WIDTH/2 cycles
				}
				last_rxd_state = cv;
			}
		
			// frame has just been received, we have nwo 128*BIT_TS_WIDTH clocks of grace 'after frame' 
			// period, according to VAN std nobody should say nothing at this time.
			debug_send_received_buf();
		
			CHECK_RST_BKR();
		}
		break;

	case 'x':
		set_txd_value(txd_val);
		tmr_prescale = 2; // set /8 prescaled, so BIT_TS_WIDTH is now 16
		init_timer0();
		zero_timer0();
		for(;;) {
			rxb_reset();

			// wait untill anything is changed on the line,
			// busy line is 1, peamble is starting from zero.
			// so actually wait for preamble
rewait_preamble:
			while ((last_rxd_state=get_rxd_value()) != 0) {
				; // do nothing
				CHECK_RST_BKR();
			}
			if (get_rxd_value() != 0)   // avoid fluctuations
				goto rewait_preamble; 

			// update state
			receiving_frame = 1;

			// run sampling clock, our clock is driven by quartz, so, ignore fronts changes 
			// as a source for clock correction
			zero_timer0();

			while (receiving_frame) {
				uint8_t tmr = get_timer0() & 0xf; // we are only interested in the last 4 bits
				
				if (tmr == 10) { // recommended sampling point is at 70% after the beginning 
					last_rxd_state = get_rxd_value();
					sample_bit();
					continue;
				}
			}
			
			debug_send_received_buf();
		
			CHECK_RST_BKR();
		}
		break;

	case '2':
		set_txd_value(txd_val);
		
		for (;;) {
			uint8_t pv=0, cv;
			rxb_byte_ptr = 0;
		
			zero_timer0(0);
			while (rxb_byte_ptr < RX_BUFFER_SIZE-3) {
				cv = get_rxd_value();
				if (cv != pv) {
					uint8_t tm = get_timer0();
					zero_timer0();
					rxb[rxb_byte_ptr] = tm;
					++rxb_byte_ptr;
					rxb[rxb_byte_ptr] = cv;
					++rxb_byte_ptr;
					pv = cv;
				}
				CHECK_RST_BKR();
			}

			debug_send_received_buf();
		}
		break;

	case '3':
		set_txd_value(txd_val);
		rxb_byte_ptr = 0;

		if (1) {
			uint8_t pv=0, cv;
			zero_timer0(0);
			for (;;) {
				cv = get_rxd_value();
				if (cv != pv) {
					uint8_t tm = get_timer0();
					zero_timer0();
					show_hex(tm);
					show_hex(cv);
					pv = cv;
					USART_vSendByte(' ');
				}
				CHECK_RST_BKR();
			}
		}
		break;

	case '4':
		set_txd_value(txd_val);
		rxb_byte_ptr = 0;

		if (1) {
			uint8_t cv, pv = 0;
			zero_timer0(0);
			for (;;) {
				uint8_t tm = get_timer0();
				cv = PIND;
				show_hex(tm);
				show_hex(cv);
				if (cv != pv)
					USART_vSendByte('\n');
				USART_vSendByte('\r');
				pv = cv;
				
				CHECK_RST_BKR();
			}
		}
		break;
	

	case 's':
		tmr_prescale = 1;
		init_timer0();
		rxb_reset();
		zero_timer0();
		if (1) {
			uint8_t tmr1, tmr2, j;
			last_rxd_state = 1;
			for (j=0; j<32; j++) {
				if ((j&0x7) == 0)
					USART_sendCrLf();
				tmr1 = get_timer0();
				sample_bit();
				tmr2 = get_timer0();
				sbclk(tmr2-tmr1);
			}
			USART_sendCrLf();
		}
		USART_vReceiveByte();	
		goto reset;

	case 'q':
		rxd_invert = rxd_invert ? 0 : (1<<PD7); //~rxd_invert;
		goto menu;
	
	case 'w':
		txd_val = !txd_val;
		goto menu;
	case 'e': 
		txd_toggle = !txd_toggle;
		goto menu;

	case 'y':
		tmr_prescale = 1; 
		goto reset;
	case 'u':
		tmr_prescale = 2; 
		goto reset;
	case 'i':
		tmr_prescale = 3; 
		goto reset;
	case 'o':
		tmr_prescale = 4; 
		goto reset;
	case 'p':
		tmr_prescale = 5; 
		goto reset;

	case 'r':
		goto reset;
	default: 
		goto menu;
	}
	return 0;
}
