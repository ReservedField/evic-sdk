# Supported vars:
# TARGET
# OBJS
# CFLAGS
# CPPFLAGS
# ASFLAGS
# LDFLAGS

# We make the following assumptions on Windows:
# arm-none-eabi gcc and binutils are compiled for Windows,
# so if you are using Cygwin, we will need path translations
# NUVOSDK must be lazily evaluated, so that we can later
# change EVICSDK when building include paths.

NUVOSDK = $(EVICSDK)/nuvoton-sdk/Library

# Force OBJS immediate expansion, since we'll be
# changing EVICSDK later.
OBJS := $(OBJS)

# We need to find out if on cygwin or not
ifeq ($(OS),Windows_NT)
	ifeq (, $(findstring cygwin, $(shell gcc -dumpmachine)))
		WIN_CYG := 0
	else
		WIN_CYG := 1
	endif
	
endif

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CC_IS_CLANG := 1
endif

ifeq ($(ARMGCC),)
    ARMGCC := $(shell cd $(shell arm-none-eabi-gcc --print-search-dir | grep 'libraries' | \
        tr '=$(if $(filter Windows_NT,$(OS)),;,:)' '\n' | \
        grep -E '/arm-none-eabi/lib/?$$' | head -1)/../.. && pwd)
endif

ifeq ($(OS),Windows_NT)
	# Always fix binutils path
	ifneq ($(ARMGCC),)
		# If using cygwin, use cygpath
		ifeq ($(WIN_CYG),1)
			ARMGCC := $(shell cygpath -w $(ARMGCC))
		endif
		
	endif
	
	ifndef CC_IS_CLANG
		NEED_FIXPATH := 1
	endif
endif

ifdef CC_IS_CLANG
	CFLAGS += -target armv7em-none-eabi -fshort-enums
else
	CC := arm-none-eabi-gcc
endif

ifdef NEED_FIXPATH
		ifeq ($(WIN_CYG), 0)
			OBJS_FIXPATH := $(OBJS)
		else
			OBJS_FIXPATH := $(shell cygpath -w $(OBJS))
			EVICSDK := $(shell cygpath -w $(EVICSDK))
		endif
	else
		OBJS_FIXPATH := $(OBJS)
endif

GCC_VERSION := $(shell arm-none-eabi-gcc -dumpversion)

AS := arm-none-eabi-as
LD := arm-none-eabi-ld
OBJCOPY := arm-none-eabi-objcopy

BINDIR := bin

INCDIRS := $(foreach d,$(shell arm-none-eabi-gcc -x c -v -E /dev/null 2>&1 | sed -n -e '/<\.\.\.>/,/End/ p' | tail -n +2 | head -n -1 | sed 's/^\s*//'),-I$d) \
	-I$(NUVOSDK)/CMSIS/Include \
	-I$(NUVOSDK)/Device/Nuvoton/M451Series/Include \
	-I$(NUVOSDK)/StdDriver/inc \
	-I$(EVICSDK)/include

LIBDIRS := -L$(ARMGCC)/arm-none-eabi/lib \
	-L$(ARMGCC)/arm-none-eabi/newlib \
	-L$(ARMGCC)/lib/arm-none-eabi/newlib \
	-L$(ARMGCC)/gcc/arm-none-eabi/$(GCC_VERSION) \
	-L$(ARMGCC)/lib/gcc/arm-none-eabi/$(GCC_VERSION) \
	-L$(EVICSDK)/lib \
	-L$(EVICSDK)/linker

CPUFLAGS := -mcpu=cortex-m4 -mthumb

ifneq ($(EVICSDK_FPU_SUPPORT),)
	CPUFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
	CFLAGS += -DEVICSDK_FPU_SUPPORT
	ASFLAGS += -DEVICSDK_FPU_SUPPORT
	LDSCRIPT := $(EVICSDK)/linker/fpu.ld
else
	LDSCRIPT := $(EVICSDK)/linker/nofpu.ld
endif

CFLAGS += -Wall $(CPUFLAGS) -Os -fdata-sections -ffunction-sections
CFLAGS += $(INCDIRS)

CPPFLAGS += -fno-exceptions -fno-rtti

ASFLAGS += $(CPUFLAGS)

# Yes, I know what I'm doing with --no-warn-mismatch.
# The thread library is compiled without FPU support to avoid issues with FPU
# context switching, which would normally result in a linker error due to
# different ABIs (soft/hard) when the SDK is compiled with FPU support. Since
# no function in the thread library accepts FP arguments, they will work fine
# together. Of course this trainwrecks when SDK is compiled with FPU support
# and APROM is not. Oh well...
LDFLAGS += $(LIBDIRS)
LDFLAGS += -nostdlib -nostartfiles -T$(LDSCRIPT) --gc-sections --no-warn-mismatch

all: env_check $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(ASFLAGS) -c -x assembler-with-cpp $< -o $@

$(TARGET).elf: $(OBJS_FIXPATH)
	test -d $(BINDIR) || mkdir $(BINDIR)
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
