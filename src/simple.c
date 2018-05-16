/* References:
 * http://codeandlife.com/2013/06/28/minimal-ps2-keyboard-on-attiny2313/
 * http://www.electronics-base.com/projects/complete-projects/108-avr-ps2-keyboard-key-readout
 * http://pinouts.ru/Inputs/KeyboardPC6_pinout.shtml
 */

#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define OUT_CLOCK_DDR DDRB
#define OUT_CLOCK_PORT PORTB
#define OUT_CLOCK_PIN 0
#define OUT_CLOCK_INPUT PINB

#define OUT_DATA_DDR DDRB
#define OUT_DATA_PORT PORTB
#define OUT_DATA_PIN 1
#define OUT_DATA_INPUT PINB

#define IN_CLOCK_DDR DDRB
#define IN_CLOCK_PORT PORTB
#define IN_CLOCK_PIN 3
#define IN_CLOCK_INPUT PINB

#define IN_DATA_DDR DDRB
#define IN_DATA_PORT PORTB
#define IN_DATA_PIN 2
#define IN_DATA_INPUT PINB

static void out_clock_high(void)
{
	OUT_CLOCK_DDR &= ~_BV(OUT_CLOCK_PIN);	// set as input
	OUT_CLOCK_PORT |= _BV(OUT_CLOCK_PIN);	// set pullup
}

static void out_clock_low(void)
{
	OUT_CLOCK_PORT &= ~_BV(OUT_CLOCK_PIN);	// zero output value
	OUT_CLOCK_DDR |= _BV(OUT_CLOCK_PIN);	// set as output
}

static void out_data_high(void)
{
	OUT_DATA_DDR &= ~_BV(OUT_DATA_PIN);	// set as input
	OUT_DATA_PORT |= _BV(OUT_DATA_PIN);	// set pullup
}

static void out_data_low(void)
{
	OUT_DATA_PORT &= ~_BV(OUT_DATA_PIN);	// zero output value
	OUT_DATA_DDR |= _BV(OUT_DATA_PIN);	// set as output
}

#define out_read_clock() (OUT_CLOCK_INPUT & (1 << OUT_CLOCK_PIN))
#define out_read_data() (OUT_DATA_INPUT & (1 << OUT_DATA_PIN))
#define in_read_data() (IN_DATA_INPUT & (1 << IN_DATA_PIN))

#define CLK_FULL 40		// 40+40 us for 12.5 kHz clock
#define CLK_HALF 20

void ps2init()
{
	out_clock_high();
	out_data_high();

	IN_CLOCK_DDR &= ~_BV(IN_CLOCK_PIN);	// set as input
	IN_DATA_DDR &= ~_BV(IN_DATA_PIN);	// set as input
}

int ps2write(unsigned char data)
{
	unsigned char i;
	unsigned char parity = 0;

	while (!out_read_clock() || !out_read_data())
		return -1;	// clock or data low

	out_data_low();
	_delay_us(CLK_HALF);

	// device sends on falling clock
	out_clock_low();
	// start bit
	_delay_us(CLK_FULL);
	out_clock_high();
	_delay_us(CLK_HALF);

	for (i = 0; i < 8; i++) {
		if (data & 1) {
			out_data_high();
			parity++;
		} else
			out_data_low();

		_delay_us(CLK_HALF);
		out_clock_low();
		_delay_us(CLK_FULL);
		out_clock_high();
		_delay_us(CLK_HALF);

		data = data >> 1;
	}

	if (parity & 1)
		out_data_low();
	else
		out_data_high();

	_delay_us(CLK_HALF);
	out_clock_low();
	_delay_us(CLK_FULL);
	out_clock_high();
	_delay_us(CLK_HALF);

	// stop bit
	out_data_high();
	_delay_us(CLK_HALF);
	out_clock_low();
	_delay_us(CLK_FULL);
	out_clock_high();
	_delay_us(CLK_HALF);

	_delay_us(50);

	return 0;
}

void ps2puts(const char *str)
{
	for (; *str != '\0'; str++) {
		while (ps2write(*str)) ;
		_delay_ms(10);
		while (ps2write(0xF0)) ;
		_delay_ms(5);
		while (ps2write(*str)) ;
		_delay_ms(30);
	}
}

#define BUFSIZE 1024
volatile int pos_input = 0;
volatile int pos_output = 0;
volatile int total_count = 0;
unsigned char buffer[BUFSIZE];

volatile int clk_time;

void queue_put(unsigned char data)
{
	// TODO use hardware memory
	buffer[pos_input % BUFSIZE] = data;
	pos_input++;
	if (total_count < BUFSIZE / 2 - 1)
		total_count++;
}

void queue_rewind()
{
	pos_output -= total_count;
	total_count = 0;
}

char queue_empty(void)
{
	return pos_input == pos_output;
}

unsigned char queue_get(void)
{
	unsigned char x = buffer[pos_output % BUFSIZE];
	pos_output++;
	return x;
}


#define BUTTON_DDR DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PIN 6
#define BUTTON_INPUT PIND

#define LED_DDR DDRD
#define LED_PORT PORTD
#define LED_PIN 7
#define LED_INPUT PIND

volatile int time_since_last_clk = 0;

ISR(PCINT1_vect)
{
	static unsigned char data, bitcount = 11;
	//if (IN_CLOCK_INPUT & _BV(IN_CLOCK_PIN)) {
	//      out_clock_high();
	//} else {
	//      out_clock_low();
	//}

	//if (in_read_data()) {
	//      out_data_high();
	//} else {
	//      out_data_low();
	//}
	if (!(IN_CLOCK_INPUT & _BV(IN_CLOCK_PIN))) {
		if (time_since_last_clk > (CLK_FULL * 2 - CLK_HALF)) {
			bitcount = 11;
		}
		time_since_last_clk = 0;

		if ((bitcount < 11) && (bitcount > 2)) {
			data = (data >> 1);
			if (in_read_data())
				data = data | 0x80;
			else
				data = data & 0x7f;
		}
		if (--bitcount == 0) {	// all bits received ?      
			queue_put(data);
			bitcount = 11;	// reset bit counter    
		}
	}
}

#define button_pressed() (!(BUTTON_INPUT & _BV(BUTTON_PIN)))

int main()
{
	ps2init();

	LED_DDR |= _BV(LED_PIN);
	BUTTON_PORT |= _BV(BUTTON_PIN);	// pullup on button

	// TODO: setup interrupt
	PCICR |= (1 << PCIE1);
	PCMSK1 |= (1 << PCINT11);
	sei();

	while (1) {
		if (!queue_empty()) {
			unsigned char x = queue_get();
			while(ps2write(x));
			LED_PORT ^= _BV(LED_PIN);
		}

		if (button_pressed() && total_count > 0) {
			LED_PORT ^= _BV(LED_PIN);
			//ps2puts("\x5a\x4d\x4b\x3a\x5a"); // \nplm\n
			ps2puts("\x5a"); // \n
			ps2puts("\x5a"); // \n
			queue_rewind();
		}
		_delay_us(1);
		time_since_last_clk++;
	}

	return 1;
}
