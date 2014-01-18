# if 1
#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#endif

typedef unsigned char uint8_t;

#define RING_BUFFER_SIZE 256 


uint8_t rb[RING_BUFFER_SIZE];

uint8_t rb_cur_bit; // Typically, it should be safe to use r2 through r7 that way.

uint8_t rb_r_ptr; // byte that should be next readed.
uint8_t rb_w_ptr; // byte that should ne next written.

uint8_t rb_error = 0;

// buffer conditions:
// w_ptr == r_ptr - buffer is empty
// w_ptr + 1 == r_ptr -- buffer is full

void rb_put_byte(uint8_t byte) 
{
	if (rb_w_ptr + 1 == rb_r_ptr) {
		rb_error = 1;
		return;
	}
	rb[rb_w_ptr++] = byte;
}

void rb_put_bit(uint8_t bit) 
{
	if (rb_w_ptr + 1 == rb_r_ptr) {
		rb_error = 1;
		return;
	}

	if (bit) 
		rb[rb_w_ptr] |= (uint8_t)rb_cur_bit;
	
	rb_cur_bit <<= 1;

	if (rb_cur_bit == 0) {
		rb_cur_bit = 1;
		rb_w_ptr ++; 
	}
}

uint8_t rb_get_byte(void)
{
	if (rb_w_ptr == rb_r_ptr) {
		rb_error = 1;
		return 0;
	}
	return rb[rb_r_ptr++];
}


/*void rxb_put_bit(uint8_t rxb_this_bit)
{
	if (~rxb_byte_ptr == 0)// will work only for 255-bytes sized buffer >= RX_BUFFER_SIZE)
		return ; // buffer overflow

	if (rxb_this_bit) 
		rxb[rxb_byte_ptr] |= (uint8_t)rxb_cur_bit;
	
	rxb_cur_bit <<= 1;

	if (rxb_cur_bit == 0) {
		rxb_cur_bit = 1;
		rxb_byte_ptr ++; 
	}
}
*/


void rb_init()
{
	rb_cur_bit = 1;
}

void test_put(uint8_t c) 
{
	char i;
	for (i=0; i<8; i++) {
		rb_put_bit(c & 1);
		c >>= 1;
	}
}

int main() 
{
	uint8_t v;

	rb_init();

	test_put('a');
	test_put('b');
	test_put('c');
	test_put('X');
	
	rb_error = 0;
	for (;;) {
		v = rb_get_byte();
		if (rb_error)
			break;
//		printf("%c-\n", v);
	}

	return 0;
}
