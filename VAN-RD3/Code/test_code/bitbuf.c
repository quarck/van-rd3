#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>


#define RX_BUFFER_SIZE 255 /*so, if rxb_ptr equals to 255, this means buffer overflowed*/
uint8_t rxb[RX_BUFFER_SIZE];

register uint8_t rxb_cur_bit asm("r2"); // Typically, it should be safe to use r2 through r7 that way.
register uint8_t rxb_byte_ptr asm("r3"); // Typically, it should be safe to use r2 through r7 that way.
register uint8_t rxb_low_bit asm("r4");
register uint8_t rxb_this_bit asm("r5");

void rxb_put_bit_int()
{
	if (~rxb_byte_ptr == 0)// will work only for 255-bytes sized buffer >= RX_BUFFER_SIZE)
		return ; // buffer overflow

	if (rxb_this_bit) 
		rxb[rxb_byte_ptr] |= (uint8_t)rxb_cur_bit;
	
	rxb_cur_bit <<= 1;

	if (rxb_cur_bit == 0) {
		rxb_cur_bit = rxb_low_bit;
		rxb_byte_ptr ++; 
	}
}

#define rxb_put_bit(x) rxb_this_bit = x; rxb_put_bit_int()

void rxb_reset()
{
	rxb_cur_bit = 1;
	rxb_byte_ptr = 0;
	rxb_low_bit = 1;
}


int main() 
{
	rxb_reset();

	rxb_put_bit(1);
	rxb_put_bit(0);
	rxb_put_bit(0);
	rxb_put_bit(0);

	rxb_put_bit(1);
	rxb_put_bit(1);
	rxb_put_bit(0);
	rxb_put_bit(0);
	
	return 0;
}
