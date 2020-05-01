# Makefile for programming the ATtiny85 via micronucleous USB
# bootloader
DEVICE      = attiny85
CLOCK      = 16000000UL
PROGRAMMER = -c usbtiny 
AVRDUDE = avrdude $(PROGRAMMER) -p $(DEVICE)
COMPILE = avr-gcc -fstack-usage -Wall -Os -std=c99 -DF_CPU=$(CLOCK) -mmcu=$(DEVICE)

TARGETS = fader.hex sokoban.hex tetris.hex snake.hex

all: $(TARGETS)

%.s : %.c
	$(COMPILE) -S -o $@ $<

%.o : %.c
	$(COMPILE) -c -o $@ $<

%.elf: %.o
	$(COMPILE) -Xlinker -Map=$@.map -o $@ $< 

%.hex: %.elf
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size --format=avr --mcu=$(DEVICE) $<

clean:
	rm -f *.hex *.elf *.o *.map
