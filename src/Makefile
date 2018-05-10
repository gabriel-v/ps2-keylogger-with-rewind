PORT ?= /dev/ttyUSB0

all: simple.lss simple.hex upload

simple.elf: *.c *.h
	avr-g++ -mmcu=atmega324a -DF_CPU=16000000 -Wall -Wl,-u,vfprintf -lprintf_flt -lm -O3 -o $@ $^

simple.hex: simple.elf
	avr-objcopy -j .text -j .data -O ihex $^ $@
	avr-size $@

simple.lss: simple.elf
	avr-objdump -t -h -S -C $^ > $@

upload: simple.hex
	sudo bootloadHID -r simple.hex

clean:
	rm -rf simple.elf simple.hex simple.lss

.PHONY: all clean upload

