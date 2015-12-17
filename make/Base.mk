# Supported vars:
# TARGET
# OBJS
# CFLAGS
# ASFLAGS
# LDFLAGS

NUVOSDK = $(EVICSDK)/nuvoton-sdk/Library

OBJS += $(EVICSDK)/src/startup/startup.o

CPU = cortex-m4

CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

BINDIR = bin

INCDIRS = -I$(NUVOSDK)/CMSIS/Include \
	-I$(NUVOSDK)/Device/Nuvoton/M451Series/Include \
	-I$(NUVOSDK)/StdDriver/inc \
	-I$(EVICSDK)/include

LDSCRIPT = $(EVICSDK)/linker/linker.ld
LIBDIRS = -L$(EVICSDK)/lib
LIBS = -levicsdk

CFLAGS += -Wall -mcpu=$(CPU) -mthumb -Os
CFLAGS += $(INCDIRS)

ASFLAGS += -mcpu=$(CPU)

LDFLAGS += -nostdlib -nostartfiles -T$(LDSCRIPT)
LDFLAGS += $(LIBDIRS)
LDFLAGS += $(LIBS)

all: $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(TARGET).elf: $(OBJS)
	mkdir -p $(BINDIR)
	$(LD) $(OBJS) $(LDFLAGS) -o $(BINDIR)/$(TARGET).elf

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary -j .text -j .data $(BINDIR)/$(TARGET).elf $(BINDIR)/$(TARGET).bin
	rm -f $(BINDIR)/$(TARGET).elf

clean:
	rm -rf $(OBJS) $(BINDIR)

.PHONY: all clean
