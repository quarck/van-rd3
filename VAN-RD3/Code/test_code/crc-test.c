#include <stdio.h>
#include <sys/types.h>
typedef u_int16_t uint16_t;
typedef u_int8_t uint8_t;


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
#define CRC_INIT  0x7fff
#define CRC_XOR_OUT 0x7fff

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

uint8_t crc_h;
uint8_t crc_l;
uint16_t accum; 

void crc_add_1()
{
	crc_h ^= 0x80;
}

void crc_proceed_bit()
{
	if (crc_h & 0x80) {
		crc_h ^= CRC_POLY >> 8;
		crc_l ^= CRC_POLY & 0xff; 
	}

	crc_h <<= 1;
	crc_h |= (crc_l >> 7);
	crc_l <<= 1;
}

void crc_init()
{
	crc_h = 0xff;//(I2) >> 8;
	crc_l = 0xfe;//(I2) & 0xff;
}
void crc_fin()
{
	crc_h ^= 0xff;
	crc_l ^= 0xfe;
}


uint16_t crc15_bt(const unsigned char *v, int len) 
{
	crc_init();
	
	while (len--) {
		int i;
		uint8_t c = *v++;
		for (i=0; i<8; i++) {
			if (c & 0x80)
				crc_add_1();
			crc_proceed_bit();
			c <<= 1;
		}
	}

	crc_fin();

	return (((uint16_t)crc_h) << 8) | (uint16_t) crc_l;
}


void bforce() 
{
	unsigned char data[10];
	uint16_t crc;
	int i0,i1,i2,i3,i4,i5,i6,len;

	for (i0=0; i0<256; i0++) { 
		printf("%d\n", i0);
		for (i1=0; i1<256; i1++) {
			for (i2=0; i2<256; i2++) {
				for (i3=0; i3<256; i3++) {
					for (len = 4; len >0; len --) {
						data[0] = i0;
						data[1] = i1; 
						data[2] = i2;
						data[3] = i3;
						crc = crc15(data,len) << 1;
						data[len] = crc >> 8;
						data[len+1] = crc & 0xff;
						if (crc15_bt(data, len+2) != 0xcc90) {
							printf("%d %d %d %d\n", i0, i1, i2, i3);
							exit(-1);
						}
					}
				}
			}
		}
	}

}



int main() 
{
	bforce();
	
	printf("%04x\n", crc15("hello", 5) <<1); 
	printf("%04x\n", crc15("good bye", 8) <<1); 
	printf("%04x\n", crc15("good byehello", 13) <<1); 

	printf("%04x\n", crc15_bt("hello\xc6\x54", 7) ); 
	printf("%04x\n", crc15_bt("good bye\x75\x0c", 10) ); 
	printf("%04x\n", crc15_bt("good byehello\xde\x9c", 15) ); 

}
