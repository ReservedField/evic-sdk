# Supported vars:
# TARGET
# OBJS
# CFLAGS
# ASFLAGS
# LDFLAGS

# We make the following assumptions on Windows:
# arm-none-eabi gcc and binutils are compiled for Windows,
# so they need path translation.
# Clang is compiled for Cygwin (no path translation).
# NUVOSDK must be lazily evaluated, so that we can later
# change EVICSDK when building include and linker paths.
# OBJS paths are Cygwin-style.

NUVOSDK = $(EVICSDK)/nuvoton-sdk/Library

# Force OBJS immediate expansion, since we'll be
# changing EVICSDK later.
OBJS := $(OBJS)
OBJS += $(EVICSDK)/src/startup/startup.o

CPU := cortex-m4

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CC_IS_CLANG := 1
endif

ifeq ($(OS),Windows_NT)
	# Always fix binutils path
	ifdef ARMGCC
		ARMGCC := $(shell cygpath -w $(ARMGCC))
	endif

	ifndef CC_IS_CLANG
		NEED_FIXPATH := 1
	endif
else
	ARMGCC := /usr
endif

ifdef CC_IS_CLANG
	CFLAGS += -target armv7em-none-eabi -fshort-enums
else
	CC := arm-none-eabi-gcc
endif

ifdef NEED_FIXPATH
	OBJS_FIXPATH := $(shell cygpath -w $(OBJS))
	EVICSDK := $(shell cygpath -w $(EVICSDK))
else
	OBJS_FIXPATH := $(OBJS)
endif

AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy

BINDIR := bin

INCDIRS := -I$(ARMGCC)/arm-none-eabi/include \
	-I$(NUVOSDK)/CMSIS/Include \
	-I$(NUVOSDK)/Device/Nuvoton/M451Series/Include \
	-I$(NUVOSDK)/StdDriver/inc \
	-I$(EVICSDK)/include

LDSCRIPT := $(EVICSDK)/linker/linker.ld

LIBDIRS := -L$(ARMGCC)/arm-none-eabi/lib \
	-L$(ARMGCC)/lib/arm-none-eabi/newlib \
	-L$(ARMGCC)/lib/gcc/arm-none-eabi/$(shell arm-none-eabi-gcc -v 2>&1 | grep '^gcc version' | awk '{print $$3}') \
	-L$(EVICSDK)/lib

LIBS := -levicsdk

CFLAGS += -Wall -mcpu=$(CPU) -mthumb -Os -fdata-sections -ffunction-sections
CFLAGS += $(INCDIRS)

ASFLAGS += -mcpu=$(CPU)

LDFLAGS += $(LIBDIRS)
LDFLAGS += $(LIBS)
LDFLAGS += -nostdlib -nostartfiles -T$(LDSCRIPT) --gc-sections

all: env_check $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(TARGET).elf: $(OBJS_FIXPATH)
	mkdir -p $(BINDIR)
	$(LD) $(OBJS_FIXPATH) $(LDFLAGS) -o $(BINDIR)/$(TARGET).elf

$(TARGET)_unencrypted.bin: $(TARGET).elf
	$(OBJCOPY) -O binary -j .text -j .data $(BINDIR)/$(TARGET).elf $(BINDIR)/$(TARGET)_unencrypted.bin
	rm -f $(BINDIR)/$(TARGET).elf

$(TARGET).bin: $(TARGET)_unencrypted.bin
	evic convert $(BINDIR)/$(TARGET)_unencrypted.bin -o $(BINDIR)/$(TARGET).bin
	rm -f $(BINDIR)/$(TARGET)_unencrypted.bin

clean:
	rm -rf $(OBJS) $(BINDIR)

env_check:
ifeq ($(ARMGCC),)
	$(error You must set the ARMGCC environment variable)
endif

.PHONY: all clean env_check
