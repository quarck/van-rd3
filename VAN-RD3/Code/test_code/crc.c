#include <stdio.h>
#include <sys/types.h>
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
#define CRC_INIT 0x7fff
#define CRC_XOR_OUT 0x7fff

typedef u_int16_t uint16_t;
uint16_t crc15(const unsigned char *v, int len) 
{
	uint16_t r = CRC_INIT;
	int i, b;

	for ( i = 0; i < len; i++ ) {
		r ^= ((uint16_t)v[i]) << 7;
		for ( b = 0; b < 8; b++ ) 
			r = (r & 0x4000) ? (r << 1) ^ CRC_POLY 
					 : (r << 1);
		r &= 0x7fff;
	}

	r ^= CRC_XOR_OUT;

	return r;
}

int main() 
{
	printf("%04x\n", crc15("\x8D\x4C\x12\x02", 4) << 1); 
}
