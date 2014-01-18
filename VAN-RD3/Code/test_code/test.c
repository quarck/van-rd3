
#include <stdio.h>

void show_hex(unsigned char c)
{
	printf("%02x", c);
}

unsigned char rxb[] = {4, 1, 2, 3, 4, 3, 0xa, 0xb, 0xc, 5, 0, 1, 0, 1, 0};
unsigned char *rxb_byte_ptr = &rxb[15];

void send_captured()
{
	unsigned char *send_ptr = &rxb[0];
	unsigned char ln;

	while (send_ptr != rxb_byte_ptr) {
		ln = *send_ptr; ++send_ptr;
		while (ln > 0) {
			if (send_ptr == rxb_byte_ptr)
				break;
			show_hex(*send_ptr);
			++send_ptr;
			--ln;
		}
		printf("\n");
	}
}
int main()
{
	send_captured();
}
