# Supported vars:
# TARGET
# OBJS
# CFLAGS
# ASFLAGS
# LDFLAGS

NUVOSDK = $(EVICSDK)/nuvoton-sdk/Library

OBJS += $(EVICSDK)/src/startup/startup.o

CPU = cortex-m4

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CFLAGS  += -target armv7em-none-eabi -fshort-enums
else
	CC := arm-none-eabi-gcc
endif

AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy

BINDIR = bin

INCDIRS = -I$(NUVOSDK)/CMSIS/Include \
	-I$(NUVOSDK)/Device/Nuvoton/M451Series/Include \
	-I$(NUVOSDK)/StdDriver/inc \
	-I$(EVICSDK)/include

LDSCRIPT = $(EVICSDK)/linker/linker.ld
LIBDIRS = -L/usr/arm-none-eabi/lib \
   -L/usr/lib/gcc/arm-none-eabi/$(shell arm-none-eabi-gcc -v 2>&1 | grep '^gcc version' | awk '{print $$3}') \
   -L$(EVICSDK)/lib
LIBS = -levicsdk

CFLAGS += -Wall -mcpu=$(CPU) -mthumb -Os
CFLAGS += $(INCDIRS)

ASFLAGS += -mcpu=$(CPU)

LDFLAGS += $(LIBDIRS)
LDFLAGS += $(LIBS)
LDFLAGS += -nostdlib -nostartfiles -T$(LDSCRIPT)

all: $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(TARGET).elf: $(OBJS)
	mkdir -p $(BINDIR)
	$(LD) $(OBJS) $(LDFLAGS) -o $(BINDIR)/$(TARGET).elf

$(TARGET)_unencrypted.bin: $(TARGET).elf
	$(OBJCOPY) -O binary -j .text -j .data $(BINDIR)/$(TARGET).elf $(BINDIR)/$(TARGET)_unencrypted.bin
	rm -f $(BINDIR)/$(TARGET).elf

$(TARGET).bin: $(TARGET)_unencrypted.bin
	evic convert $(BINDIR)/$(TARGET)_unencrypted.bin -o $(BINDIR)/$(TARGET).bin
	rm -f $(BINDIR)/$(TARGET)_unencrypted.bin

clean:
	rm -rf $(OBJS) $(BINDIR)

.PHONY: all clean
