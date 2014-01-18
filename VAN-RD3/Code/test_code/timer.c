#include <avr/io.h>
#include <avr/iom8.h>

#define BIT_TS_WIDTH 128


void init_timer0() 
{
	// set timer init value
	TCNT0 = 0;

	// init timer, set timer pre-scaler to 'not used'
	TCCR0 = (1 << CS00);
}

#define get_timer0() TCNT0
#define zero_timer() TCNT0 = (uint8_t) 0

void gav_gav() 
{
	static volatile uint8_t v = 0;
	if (v) {
		PORTC &= ~(1<<PC5); /* LED on */
		PORTC |= 1<<PC4; /* LED off */
	} else {
		PORTC |= 1<<PC5; /* LED off */
		PORTC &= ~(1<<PC4); /* LED on */
	}
	v = !v;
}

void sample_bit() 
{
	static volatile uint16_t cnt = 0;
	cnt ++;
	if (cnt == 0xffff)
		gav_gav();
}

int main(void) 
{
	init_timer0();

	// count each BIT_TS_WIDTH-clock length intervals, starting at some random point...
	zero_timer();
	while(1) {
		uint8_t v = get_timer0();
		if (v < BIT_TS_WIDTH)
			continue;
		zero_timer();
		
		sample_bit(); // so, this will rise 125000 times per second (approx) 
	}
	return 0;
}
