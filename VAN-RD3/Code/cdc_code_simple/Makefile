default: cdc.hex 

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

%.obj : cdc.o tx.o
	$(CC) $(CFLAGS) cdc.o tx.o -o $@

cdc.hex : cdc.obj 
	$(OBJ2HEX) -j .text -O ihex  $< $@

%.bin : cdc.obj
	$(OBJ2HEX) -j .text -O binary $< $@

clean :
	rm -f *.hex *.obj *.o *.i *.s

crc:	crc.c
	gcc -Wall crc.c -o crc

none:
