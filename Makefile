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

OUTDIR = lib
DOCDIR = doc

CPU = cortex-m4

ifeq ($(shell $(CC) -v 2>&1 | grep -c "clang version"), 1)
	CFLAGS += -target armv7em-none-eabi -fshort-enums
else
	CC := arm-none-eabi-gcc
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

all: $(TARGET).a

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
	rm -rf $(OBJS) $(OUTDIR)/$(TARGET).a $(OUTDIR) $(DOCDIR)

.PHONY: all clean docs
