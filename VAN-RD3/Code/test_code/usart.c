#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>


void delayms(uint16_t millis) 
{
	uint16_t loop;
	while ( millis ) {
		_delay_ms(1);
		millis--;
	}
}

// Define baud rate

// all baudr values are for 16Mhz clock, see pg162, table 63 of 
// mega 8 datasheet.

// modes for U2X=0
//#define USART_UBBR_VALUE 8 /* 115.2k (-3.5% error).*/
//#define USART_UBBR_VALUE 3 /* 250k (0% error)*/
//#define USART_UBBR_VALUE 1 /* 500k (0% error)*/
#define USART_UBBR_VALUE 25 /* 38.4, 0.2% */
//#define USART_UBR_VALUE 103 /* 9600, 0.2% */
//#define USART_UBR_VALUE 416 /* 2400, -0.1% */

// modes for U2X=1
//#define USART_UBBR_VALUE 832 /* 2400, 0.0% */





void USART_vInit(void)
{
// When the function writes to the UCSRC Register, the URSEL bit (MSB) must be set due 
// to the sharing of I/O location by UBRRH and UCSRC.

	// Set frame format to 8 data bits, no parity, 1 stop bit
	UCSRC = (1<<URSEL) | (0<<USBS)|(1<<UCSZ1)|(1<<UCSZ0);

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
	UCSRB = (1<<RXEN)|(1<<TXEN);
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

#define POLY 0x53a9
#define POLY_H 0x53
#define POLY_L 0xa9


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

inline void USART_sendString(const char *str) 
{
	while (*str) 
		USART_vSendByte(*str++);
}

#define LED_ON(x) PORTC &= ~(1<<x)
#define LED_OFF(x) PORTC |= (1<<x)

inline void led_show_num(uint8_t num) 
{
	if (num > 0x63)
		num = 0x63; // absolute maximum for 6 leds
	if (num & 0x20) LED_ON(PC5); else LED_OFF(PC5);
	if (num & 0x10) LED_ON(PC4); else LED_OFF(PC4);
	if (num & 0x08) LED_ON(PC3); else LED_OFF(PC3);
	if (num & 0x04) LED_ON(PC2); else LED_OFF(PC2);
	if (num & 0x02) LED_ON(PC1); else LED_OFF(PC1);
	if (num & 0x01) LED_ON(PC0); else LED_OFF(PC0);
}

inline void led_init()
{
	DDRC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5);/* set PC0-PC5 to output */
}

int main(void)
{
	uint8_t u8Data;
	uint8_t i;

	// init leds
	led_init();

	// Initialise USART
	USART_vInit();
 

	for (i=0; i<64; i++) {
		led_show_num(i);
		delayms(40);
	}

	// Send string
	USART_vSendByte('A');
	USART_sendString("\nHello world!\n");
	
	// Repeat indefinitely
	for(i=0; ; i++) {
		u8Data = USART_vReceiveByte();
		USART_vSendByte(u8Data);
		led_show_num(i & 0x3f);
	}
}


void EEPROM_write(unsigned int uiAddress, unsigned char ucData)
{
  /* Wait for completion of previous write */
  while(EECR & (1<<EEWE))
    ;
  /* Set up address and data registers */
  EEAR = uiAddress;
  EEDR = ucData;
  /* Write logical one to EEMWE */
  EECR |= (1<<EEMWE);
  /* Start eeprom write by setting EEWE */
  EECR |= (1<<EEWE);
}


unsigned char EEPROM_read(unsigned int uiAddress)
{
  /* Wait for completion of previous write */
  while(EECR & (1<<EEWE))
    ;
  /* Set up address register */
  EEAR = uiAddress;
  /* Start eeprom read by writing EERE */
  EECR |= (1<<EERE);
  /* Return data from data register */
  return EEDR;
}

