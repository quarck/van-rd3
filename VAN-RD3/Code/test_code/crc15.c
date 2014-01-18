
/*
 * two variants of CRC-15 implementation: first is using 16-bit (or higher) 
 * width variables, and the second is using only u_int8_t-s, so to get 
 * working code suitable for MCU.
 */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#define POLY 0x53a9
#define POLY_H 0x53
#define POLY_L 0xa9


u_int16_t crc15_a(const unsigned char *v, int len) 
{
	u_int16_t r = 0;
	int i, b;
	
	for ( i = 0; i < len; i++ ) {
		r |= *v++;
		for ( b = 0; b < 8; b++ ) 
			r = (r & 0x4000) ? (r << 1) ^ POLY
					 : (r << 1);
		r &= 0x7fff;
	}

	return r;
}

u_int16_t crc15_b(const unsigned char *v, int len) 
{
	u_int8_t rh = 0, rl = 0;

	u_int8_t i, b;
	
	for ( i = 0; i < len; i++ ) {
		rl |= *v++;
		for ( b = 0; b < 8; b++ ) {
			rh <<= 1;
			rh |= (rl & 0x80) ? 1 : 0 ;
			rl <<= 1;
			if (rh & 0x80) { // shifted 0x40 
				rh ^= POLY_H;
				rl ^= POLY_L;
			}
		}
		rh &= 0x7f;
	}

	return (((u_int16_t)rh) << 8 ) | (u_int16_t)rl;
}


int main() 
{
	int i;
	const char *test_strings[] = 
				{
					"123456789",
					"abc",
					"", 
					"hello world",
					"summer dies, summer ends...",
					"just a long text test"
				};
	for (i=0; i<sizeof(test_strings)/sizeof(test_strings[0]); i++) {
		const char *str = test_strings[i];
		int len = strlen(str);
		printf("Comparing %s:\n\tA:%04x\n\tB:%04x\n", str, 
				crc15_a(str, len), 
				crc15_b(str, len));
	}

	return 0;
}
