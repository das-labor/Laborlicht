# Default values
OBJ            = main.o
OUT           ?= image
MCU_TARGET    ?= atmega8
MCU_CC        ?= avr-gcc
OPTIMIZE      ?= -Os
WARNINGS      ?= -Wall
DEFS          ?= -DF_CPU=16000000
CFLAGS        += -MMD -g -std=c99 -mmcu=$(MCU_TARGET) $(OPTIMIZE) $(WARNINGS) $(DEFS)
ASFLAGS       ?= -g $(DEFS)
LDFLAGS       ?= -Wl,-Map,$(OUT).map
PROG          ?= usbasp
PROG_DEV      ?= # e.g. "/dev/ttyUSB0"
PROG_BAUD     ?= # e.g. "19200"

# External Tools
OBJCOPY       ?= avr-objcopy
OBJDUMP       ?= avr-objdump
FLASH         ?= avrdude -c $(PROG) \
		 -p $(MCU_TARGET) \
		 $(if $(PROG_DEV),-P $(PROG_DEV)) \
		 $(if $(PROG_BAUD),-b $(PROG_BAUD)) \
		 -U flash:w:$(OUT).hex:i

#############################################################################
# Rules
all: $(OUT).elf lst text eeprom

clean:
	rm -rf $(OUT) *.o *.d *.lst *.map $(OUT).hex $(OUT)_eeprom.hex *.bin *.srec
	rm -rf *.srec $(OUT).elf

flash: $(OUT).hex
	$(FLASH)

#############################################################################
# Building Rules
$(OUT).elf: $(OBJ)
	$(MCU_CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(MCU_CC) $(CFLAGS) -c $<

%.o: %.S
	$(MCU_CC) -mmcu=$(MCU_TARGET) $(ASFLAGS) -c $<

lst: $(OUT).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $@

# Rules for building the .text rom images
text: hex bin srec

hex:  $(OUT).hex
bin:  $(OUT).bin
srec: $(OUT).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(OUT)_eeprom.hex
ebin:  $(OUT)_eeprom.bin
esrec: $(OUT)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@

DEPS := $(wildcard *.d)
ifneq ($(DEPS),)
include $(DEPS)
endif
