TARGET := libevicsdk
TARGET_CRT0 := $(TARGET)_crt0

# We make the following assumptions on Windows:
# arm-none-eabi gcc and binutils are compiled for Windows,
# so if you are using Cygwin, we will need path translations
# NUVOSDK must be lazily evaluated, so that we can later
# change EVICSDK when building include paths.

# Small fix to bug where cygpath -w mistranslates paths with mixed slashes (/, \)
EVICSDK := $(subst \,/,$(EVICSDK))
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
	src/startup/sleep.o \
	src/sysinfo/SysInfo.o \
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
		ifeq ($(WIN_CYG), 0)
			OBJS_FIXPATH := $(OBJS)
			OBJS_CRT0_FIXPATH := $(OBJS_CRT0)
		else
			OBJS_FIXPATH := $(shell cygpath -w $(OBJS))
			OBJS_CRT0_FIXPATH := $(shell cygpath -w $(OBJS_CRT0))
			EVICSDK := $(shell cygpath -w $(EVICSDK))
		endif
	else
		OBJS_FIXPATH := $(OBJS)
		OBJS_CRT0_FIXPATH := $(OBJS_CRT0)
	endif
endif

ifeq ($(WIN_CYG),0)
	SDKTAG := $(shell git describe --abbrev --dirty --always --tags 2> NUL ) # Fix for Windows w/o cygwin (NUL instead of /dev/null)
else
	SDKTAG := $(shell git describe --abbrev --dirty --always --tags 2> /dev/null ) 
endif
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
	test -d $(OUTDIR) || mkdir $(OUTDIR)
	$(AR) -rv $(OUTDIR)/$(TARGET).a $(OBJS_FIXPATH)

$(TARGET_CRT0).o: $(OBJS_CRT0_FIXPATH)
	test -d $(OUTDIR) || mkdir $(OUTDIR)
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
ifeq ($(WIN_CYG),0)
		@printf ".section .evicsdk_tag\n.asciz \"evic-sdk-$(SDKTAG)\"\n" > $(TAGNAME).s
else
		@printf '.section .evicsdk_tag\n.asciz "evic-sdk-$(SDKTAG)"\n' > $(TAGNAME).s
endif


.PHONY: all clean docs env_check gen_tag
