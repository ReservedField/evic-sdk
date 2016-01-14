TARGET = libevicsdk

NUVOSDK = $(EVICSDK)/nuvoton-sdk/Library

OBJS = $(NUVOSDK)/Device/Nuvoton/M451Series/Source/system_M451Series.o \
	$(NUVOSDK)/StdDriver/src/clk.o \
	$(NUVOSDK)/StdDriver/src/fmc.o \
	$(NUVOSDK)/StdDriver/src/gpio.o \
	$(NUVOSDK)/StdDriver/src/spi.o \
	$(NUVOSDK)/StdDriver/src/sys.o \
	$(NUVOSDK)/StdDriver/src/timer.o \
	$(NUVOSDK)/StdDriver/src/usbd.o \
	$(NUVOSDK)/StdDriver/src/eadc.o \
	src/startup/init.o \
	src/dataflash/Dataflash.o \
	src/display/Display_SSD.o \
	src/display/Display_SSD1306.o \
	src/display/Display_SSD1327.o \
	src/display/Display.o \
	src/font/Font_DejaVuSansMono_8pt.o \
	src/timer/Timer.o \
	src/button/Button.o \
	src/usb/USB_VirtualCOM.o \
	src/adc/ADC.o \
	src/battery/Battery.o

AEABI_OBJS = src/aeabi/aeabi_memset-thumb2.o \
	src/aeabi/aeabi_memclr.o

OUTDIR = lib
DOCDIR = doc

CPU = cortex-m4

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CFLAGS += -target armv7em-none-eabi -fshort-enums

	AEABI_COUNT = $(shell arm-none-eabi-nm -g /usr/arm-none-eabi/lib/armv7e-m/libc.a | grep -Ec 'T __aeabi_mem(set|clr)[48]?$$')
	ifeq ($(AEABI_COUNT), 0)
		# __aeabi_memset* and __aeabi_memclr* are not exported by libc
		# We provide our own implementations
		OBJS += $(AEABI_OBJS)
	else ifneq ($(AEABI_COUNT), 6)
		# Only part of __aeabi_memset* and __aeabi_memclr* are exported by libc
		# This should never happen, bail out in __aeabi_check
		AEABI_ERROR = 1
	endif
else
	CC = arm-none-eabi-gcc
endif

AS = arm-none-eabi-as
AR = arm-none-eabi-ar
OBJCOPY = arm-none-eabi-objcopy

INCDIRS  = -I$(NUVOSDK)/CMSIS/Include
INCDIRS += -I$(NUVOSDK)/Device/Nuvoton/M451Series/Include
INCDIRS += -I$(NUVOSDK)/StdDriver/inc
INCDIRS += -Iinclude

CFLAGS += -Wall -mcpu=$(CPU) -mthumb -Os
CFLAGS += $(INCDIRS)

ASFLAGS = -mcpu=$(CPU)

all: __aeabi_check $(TARGET).a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

$(TARGET).a: $(OBJS)
	mkdir -p $(OUTDIR)
	$(AR) -rv $(OUTDIR)/$(TARGET).a $(OBJS)

docs:
	doxygen

clean:
	rm -rf $(OBJS) $(AEABI_OBJS) $(OUTDIR)/$(TARGET).a $(OUTDIR) $(DOCDIR)

__aeabi_check:
ifneq ($(AEABI_ERROR),)
	$(error Your libc is exporting only part of __aeabi symbols)
endif

.PHONY: all clean docs __aeabi_check
