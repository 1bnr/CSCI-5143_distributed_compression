PORT=/dev/ttyACM*
VIRTUAL_SERIAL_PATH=../virtualserial

MCU=atmega32u4
CFLAGS+= -g -Wall -mcall-prologues -mmcu=$(MCU) -Os -std=c99
CFLAGS+= -I$(VIRTUAL_SERIAL_PATH) -DVIRTUAL_SERIAL

LDFLAGS+= -Wl,-gc-sections -Wl,-relax
LDFLAGS+=  -L$(VIRTUAL_SERIAL_PATH) -lVirtualSerial

CC=avr-gcc
TARGET=main
SRCS = $(wildcard *.c)
OBJECT_FILES= $(SRCS:.c=.o)

all: $(TARGET).hex

clean:
	rm -f *.o *.hex *.obj *.hex

%.hex: %.obj
	avr-objcopy -R .eeprom -O ihex $< $@

%.obj: $(OBJECT_FILES)
	$(CC) $(CFLAGS) $(OBJECT_FILES) $(LDFLAGS) -o $@

program: $(TARGET).hex
	avrdude -p $(MCU) -c avr109 -P $(PORT) -U flash:w:$(TARGET).hex
