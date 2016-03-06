TARGET := libevicsdk
TARGET_CRT0 := $(TARGET)_crt0

# We make the following assumptions on Windows:
# arm-none-eabi gcc and binutils are compiled for Windows,
# so they need path translation.
# Clang is compiled for Cygwin (no path translation).
# NUVOSDK must be lazily evaluated, so that we can later
# change EVICSDK when building include paths.
# We want to keep OBJS with Cygwin paths for clean target.

NUVOSDK = $(EVICSDK)/nuvoton-sdk/Library

OBJS := $(NUVOSDK)/Device/Nuvoton/M451Series/Source/system_M451Series.o \
	$(NUVOSDK)/StdDriver/src/clk.o \
	$(NUVOSDK)/StdDriver/src/fmc.o \
	$(NUVOSDK)/StdDriver/src/gpio.o \
	$(NUVOSDK)/StdDriver/src/spi.o \
	$(NUVOSDK)/StdDriver/src/sys.o \
	$(NUVOSDK)/StdDriver/src/timer.o \
	$(NUVOSDK)/StdDriver/src/usbd.o \
	$(NUVOSDK)/StdDriver/src/eadc.o \
	$(NUVOSDK)/StdDriver/src/pwm.o \
	src/startup/initfini.o \
	src/startup/sbrk.o \
	src/startup/init.o \
	src/dataflash/Dataflash.o \
	src/display/Display_SSD.o \
	src/display/Display_SSD1306.o \
	src/display/Display_SSD1327.o \
	src/display/Display.o \
	src/font/Font_DejaVuSansMono_8pt.o \
	src/timer/TimerUtils.o \
	src/button/Button.o \
	src/usb/USB_VirtualCOM.o \
	src/adc/ADC.o \
	src/battery/Battery.o \
	src/atomizer/Atomizer.o

TAGNAME := src/startup/evicsdk_tag
OBJS_CRT0 := src/startup/startup.o \
	$(TAGNAME).o

AEABI_OBJS := src/aeabi/aeabi_memset-thumb2.o \
	src/aeabi/aeabi_memclr.o

OUTDIR := lib
DOCDIR := doc

CPU := cortex-m4

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CC_IS_CLANG := 1
endif

ifeq ($(OS),Windows_NT)
	# Always fix binutils path
	ifneq ($(ARMGCC),)
		ARMGCC := $(shell cygpath -w $(ARMGCC))
	endif

	ifndef CC_IS_CLANG
		NEED_FIXPATH := 1
	endif
else ifeq ($(ARMGCC),)
	# Allow override but default to /usr
	ARMGCC := /usr
endif

ifneq ($(ARMGCC),)
	ifdef CC_IS_CLANG
		CFLAGS += -target armv7em-none-eabi -fshort-enums

		AEABI_COUNT := $(shell arm-none-eabi-nm -g $(ARMGCC)/arm-none-eabi/lib/armv7e-m/libc.a | grep -Ec 'T __aeabi_mem(set|clr)[48]?$$')
		ifeq ($(AEABI_COUNT), 0)
			# __aeabi_memset* and __aeabi_memclr* are not exported by libc
			# We provide our own implementations
			OBJS += $(AEABI_OBJS)
		else ifneq ($(AEABI_COUNT), 6)
			# Only part of __aeabi_memset* and __aeabi_memclr* are exported by libc
			# This should never happen, bail out in env_check
			AEABI_ERROR := 1
		endif
	else
		CC := arm-none-eabi-gcc
	endif

	ifdef NEED_FIXPATH
		OBJS_FIXPATH := $(shell cygpath -w $(OBJS))
		OBJS_CRT0_FIXPATH := $(shell cygpath -w $(OBJS_CRT0))
		EVICSDK := $(shell cygpath -w $(EVICSDK))
	else
		OBJS_FIXPATH := $(OBJS)
		OBJS_CRT0_FIXPATH := $(OBJS_CRT0)
	endif
endif

SDKTAG := $(shell git describe --abbrev --dirty --always --tags 2> /dev/null)
ifeq ($(SDKTAG),)
	SDKTAG := unknown
endif

AS := arm-none-eabi-as
LD := arm-none-eabi-ld
AR := arm-none-eabi-ar
OBJCOPY := arm-none-eabi-objcopy

INCDIRS := -I$(NUVOSDK)/CMSIS/Include \
	-I$(NUVOSDK)/Device/Nuvoton/M451Series/Include \
	-I$(NUVOSDK)/StdDriver/inc \
	-Iinclude

CFLAGS += -Wall -mcpu=$(CPU) -mthumb -Os -fdata-sections -ffunction-sections
CFLAGS += $(INCDIRS)

ASFLAGS := -mcpu=$(CPU)

all: env_check gen_tag $(TARGET_CRT0).o $(TARGET).a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(TARGET).a: $(OBJS_FIXPATH)
	mkdir -p $(OUTDIR)
	$(AR) -rv $(OUTDIR)/$(TARGET).a $(OBJS_FIXPATH)

$(TARGET_CRT0).o: $(OBJS_CRT0_FIXPATH)
	mkdir -p $(OUTDIR)
	$(LD) -r $(OBJS_CRT0_FIXPATH) -o $(OUTDIR)/$(TARGET_CRT0).o

docs:
	doxygen

clean:
	rm -rf $(OBJS) $(OBJS_CRT0) $(AEABI_OBJS) $(OUTDIR)/$(TARGET).a $(OUTDIR)/$(TARGET_CRT0).o $(OUTDIR) $(DOCDIR)

env_check:
ifeq ($(ARMGCC),)
	$(error You must set the ARMGCC environment variable)
endif
ifneq ($(AEABI_ERROR),)
	$(error Your libc is exporting only part of __aeabi symbols)
endif

gen_tag:
	@rm -f $(TAGNAME).s $(TAGNAME).o
	@printf '.section .evicsdk_tag\n.asciz "evic-sdk-$(SDKTAG)"\n' > $(TAGNAME).s

.PHONY: all clean docs env_check gen_tag
