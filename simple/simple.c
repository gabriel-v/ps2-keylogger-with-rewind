#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>

int main() {
	DDRD |= (1 << PD7);

	while(1) {
		PORTD ^= (1 << PD7);
                _delay_ms(500);
		PORTD ^= (1 << PD7);
                _delay_ms(1500);
		PORTD ^= (1 << PD7);
                _delay_ms(250);
	}
	return 0;
}

