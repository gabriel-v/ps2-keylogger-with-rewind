#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>

#define PS2_CLOCK_DDR DDRB
#define PS2_CLOCK_PORT PORTB
#define PS2_CLOCK_PIN 0
#define PS2_CLOCK_INPUT PINB

#define PS2_DATA_DDR DDRB
#define PS2_DATA_PORT PORTB
#define PS2_DATA_PIN 1
#define PS2_DATA_INPUT PINB

static void clockHigh(void) {
        PS2_CLOCK_DDR &= ~_BV(PS2_CLOCK_PIN); // set as input
        PS2_CLOCK_PORT |= _BV(PS2_CLOCK_PIN); // set pullup
}

static void clockLow(void) {
        PS2_CLOCK_PORT &= ~_BV(PS2_CLOCK_PIN); // zero output value
        PS2_CLOCK_DDR |= _BV(PS2_CLOCK_PIN); // set as output
}

static void dataHigh(void) {
        PS2_DATA_DDR &= ~_BV(PS2_DATA_PIN); // set as input
        PS2_DATA_PORT |= _BV(PS2_DATA_PIN); // set pullup
}

static void dataLow(void) {
        PS2_DATA_PORT &= ~_BV(PS2_DATA_PIN); // zero output value
        PS2_DATA_DDR |= _BV(PS2_DATA_PIN); // set as output
}

#define readClockPin() (PS2_CLOCK_INPUT & (1 << PS2_CLOCK_PIN))
#define readDataPin() (PS2_DATA_INPUT & (1 << PS2_DATA_PIN))

#define CLK_FULL 40 // 40+40 us for 12.5 kHz clock
#define CLK_HALF 20

void ps2init() {
        clockHigh();
        dataHigh();
}

int ps2write(unsigned char data) {
  unsigned char i;
  unsigned char parity = 0;
        
  if(!readClockPin() || !readDataPin())
          return -1; // clock or data low

  dataLow();
  _delay_us(CLK_HALF);

  // device sends on falling clock
  clockLow();   // start bit
  _delay_us(CLK_FULL);
  clockHigh();
  _delay_us(CLK_HALF);

  for(i=0; i<8; i++) {
          if(data & 1) {
                  dataHigh();
                  parity++;
          } else
                  dataLow();

          _delay_us(CLK_HALF);
          clockLow();   
          _delay_us(CLK_FULL);
          clockHigh();
          _delay_us(CLK_HALF);

          data = data >> 1;
  }

  if(parity & 1)
          dataLow();
  else
          dataHigh();

  _delay_us(CLK_HALF);
  clockLow();   
  _delay_us(CLK_FULL);
  clockHigh();
  _delay_us(CLK_HALF);

  // stop bit
  dataHigh();
  _delay_us(CLK_HALF);
  clockLow();   
  _delay_us(CLK_FULL);
  clockHigh();
  _delay_us(CLK_HALF);

  _delay_us(50);

  return 0;
}

void ps2puts(const char *str) {
	for(; *str != '\0'; str++) {
		while(ps2write(*str));
		_delay_ms(10);
		while(ps2write(0xF0));
		_delay_ms(5);
		while(ps2write(*str));
		_delay_ms(30);
	}
}


#define BUTTON_DDR DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PIN 6
#define BUTTON_INPUT PIND

int main() {
        ps2init();

        BUTTON_PORT |= _BV(BUTTON_PIN); // pullup on button
	DDRD |= (1 << PD7);

        while(1) {
		PORTD ^= (1 << PD7);
                if(!(BUTTON_INPUT & _BV(BUTTON_PIN))) {
			ps2puts("\x4d\x4b\x3a\x5a");
		}
		_delay_ms(200);
        }

        return 1;
}
