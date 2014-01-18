#define F_CPU 10000000UL
#include <avr/io.h>
#include <util/delay.h>

void delayms(uint16_t millis) {
  uint16_t loop;
  while ( millis ) {
    _delay_ms(1);
    millis--;
  }
}

int main(void) {
  DDRC |= 1<<PC5; /* set PB0 to output */
  DDRC |= 1<<PC4; /* set PB0 to output */
  while(1) {
    PORTC &= ~(1<<PC5); /* LED on */
    PORTC |= 1<<PC4; /* LED off */
    delayms(100);
    PORTC |= 1<<PC5; /* LED off */
    PORTC &= ~(1<<PC4); /* LED on */
    delayms(900);
  }
  return 0;
}
