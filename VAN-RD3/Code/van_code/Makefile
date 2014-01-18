default: van.hex 

CC=avr-gcc 
OBJ2HEX=avr-objcopy

COMMON = -mmcu=atmega8
CFLAGS=-Wall $(COMMON) -save-temps -Os
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2


## Compile options common for all C compilation units.

tx.o: ./tx.S
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

%.obj : van.o tx.o
	$(CC) $(CFLAGS) van.o tx.o -o $@

van.hex : van.obj 
	$(OBJ2HEX) -j .text -O ihex  $< $@

%.bin : van.obj
	$(OBJ2HEX) -j .text -O binary $< $@

clean :
	rm -f *.hex *.obj *.o *.i *.s

crc:	crc.c
	gcc -Wall crc.c -o crc

none:
