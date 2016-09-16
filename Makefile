# This file is part of eVic SDK.
#
# eVic SDK is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# eVic SDK is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
#
# Copyright (C) 2016 ReservedField

include $(EVICSDK)/make/Helper.mk
include $(EVICSDK)/make/Common.mk

# Output targets.
TARGET_SDK := libevicsdk
TARGET_NUVO := libnuvosdk
TARGET_CRT0 := evicsdk-crt0

# SDK objects that are always compiled as no-FPU.
OBJS_SDK_NOFPU := \
	src/thread/Thread.o \
	src/thread/Queue.o

# SDK objects.
OBJS_SDK := \
	src/startup/initfini.o \
	src/startup/sbrk.o \
	src/startup/init.o \
	src/startup/mainthread.o \
	src/startup/sleep.o \
	src/sysinfo/SysInfo.o \
	src/dataflash/Dataflash.o \
	src/display/Display_SSD.o \
	src/display/Display_SSD1306.o \
	src/display/Display_SSD1327.o \
	src/display/Display.o \
	src/font/Font_DejaVuSansMono_8pt.o \
	src/timer/TimerUtils.o \
	src/rtc/RTCUtils.o \
	src/button/Button.o \
	src/usb/USB_VirtualCOM.o \
	src/adc/ADC.o \
	src/battery/Battery.o \
	src/atomizer/Atomizer.o \
	$(OBJS_SDK_NOFPU)

# SDK objects to build in case of missing __aeabi functions.
OBJS_SDK_AEABI := \
	src/aeabi/aeabi_memset-thumb2.o \
	src/aeabi/aeabi_memclr.o

# Nuvoton SDK roots (relative).
NUVOSDK_LOCAL := nuvoton-sdk/Library
NUVOSDK_DEVSRC := $(NUVOSDK_LOCAL)/Device/Nuvoton/M451Series/Source
NUVOSDK_STDSRC := $(NUVOSDK_LOCAL)/StdDriver/src

# Nuvoton SDK objects.
OBJS_NUVO := \
	$(NUVOSDK_DEVSRC)/system_M451Series.o \
	$(NUVOSDK_STDSRC)/clk.o \
	$(NUVOSDK_STDSRC)/fmc.o \
	$(NUVOSDK_STDSRC)/gpio.o \
	$(NUVOSDK_STDSRC)/spi.o \
	$(NUVOSDK_STDSRC)/sys.o \
	$(NUVOSDK_STDSRC)/timer.o \
	$(NUVOSDK_STDSRC)/rtc.o \
	$(NUVOSDK_STDSRC)/usbd.o \
	$(NUVOSDK_STDSRC)/eadc.o \
	$(NUVOSDK_STDSRC)/pwm.o

# SDK tag object file.
SDKTAG_OBJ := src/startup/sdktag.o

# Crt0 objects.
OBJS_CRT0 := \
	src/startup/startup.o \
	src/thread/ContextSwitch.o \
	$(SDKTAG_OBJ) \
	$(if $(EVICSDK_FPU_DISABLE),,src/thread/UsageFault_fpu.o)

# Crt0 objects only compiled in debug builds.
OBJS_CRT0_DBG := src/startup/fault.o

# Extra C/C++ flags for SDK/crt0 compilation.
# TODO: fixup warnings for GCC and Clang before enabling this.
#       Also need -Wno-bitwise-op-parentheses for Clang + Nuvo SDK.
#CCFLAGS_EXTRA := -Wall

# Find path to libc. Using find would be better, but on Cygwin we'd have to
# convert the native style library paths to Cygwin style. This hack avoids it
# by (ab)using binutils to do the search.
LIBC_PATH_FIXBU := $(shell \
	arm-none-eabi-ld --verbose $(foreach d,$(ARM_LIBDIRS_FIXBU),-L$d) -lc \
		$(NULLDEV_TC) 2>&1 | \
	sed -n 's/attempt to open \(.*libc\.a\) succeeded/\1/p' | head -1)
ifndef LIBC_PATH_FIXBU
$(error Could not detect libc path)
endif
ifdef EVICSDK_MAKE_DEBUG
$(info Libc path (raw): $(LIBC_PATH_FIXBU))
$(info Libc path (canonical): \
	$(call path-canon,$(call unfixpath-tc,$(LIBC_PATH_FIXBU))))
endif

# Old newlib versions don't have __aeabi_memset* and __aeabi_memclr* (added in
# commit 24e054c). If this is the case, we bake our __aeabi objects into the
# SDK to avoid undefined references (especially with Clang).
AEABI_COUNT := $(shell arm-none-eabi-nm -g $(LIBC_PATH_FIXBU) | \
	grep -Ec 'T __aeabi_mem(set|clr)[48]?$$')
ifndef AEABI_COUNT
$(error Could not detect state of __aeabi symbols)
endif
$(if $(EVICSDK_MAKE_DEBUG),$(info __aeabi count: $(AEABI_COUNT)))
ifeq ($(AEABI_COUNT),0)
	OBJS_SDK += $(OBJS_SDK_AEABI)
else ifneq ($(AEABI_COUNT),6)
$(error Libc is exporting only part of __aeabi symbols)
endif

# Generates the SDK tag from git. Since this is an expensive operation, the
# result is cached after the first invocation.
get-sdktag = $(or $(__SDKTAG),$(eval __SDKTAG := $(__get-sdktag))$(__SDKTAG))
__get-sdktag = evic-sdk-$(or $(strip $(shell \
		git describe --abbrev --dirty --always --tags 2> $(NULLDEV))),unknown)
$(if $(EVICSDK_MAKE_DEBUG),$(info SDK tag: $(get-sdktag)))

# All objects, excluding debug-only objects.
OBJS_ALL := $(OBJS_SDK) $(OBJS_NUVO) $(OBJS_CRT0)
# All debug-only objects.
OBJS_ALL_DBG := $(OBJS_CRT0_DBG)

# Documentation output directory.
DOCDIR := doc

# Output directory clean template.
clean-sdkdir-tmpl = $(call clean-dir-tmpl,$1,$2,$(SDKDIR))
# Library output path template. Extra argument: library name.
lib-tmpl = $(call sdkdir-tmpl,$1,$2)/$3.a
# SDK library output path template.
sdk-tmpl = $(call lib-tmpl,$1,$2,$(TARGET_SDK))
# Nuvoton SDK library output path template.
nuvo-tmpl = $(call lib-tmpl,$1,$2,$(TARGET_NUVO))
# Crt0 object output path template
crt0-tmpl = $(call sdkdir-tmpl,$1,$2)/$(TARGET_CRT0).o

# Object targets for all devices and flavors.
objs-all := $(call tmpl-all,objs-tmpl,$(OBJS_ALL)) \
	$(call tmpl-flavor,objs-tmpl,$(BUILD_FLAVOR_DBG),$(OBJS_ALL_DBG))
# SDK library output paths for all devices and flavors.
sdk-all := $(call tmpl-all,sdk-tmpl)
# Nuvoton SDK library output paths for all device and flavors.
nuvo-all := $(call tmpl-all,nuvo-tmpl)
# All library output paths for all devices and flavors.
lib-all := $(sdk-all) $(nuvo-all)
# Crt0 object output paths for all devices and flavors.
crt0-all := $(call tmpl-all,crt0-tmpl)
# Crt0 object output paths for all devices, debug flavor.
crt0-dbg := $(call tmpl-flavor,crt0-tmpl,$(BUILD_FLAVOR_DBG))
# No-FPU objects for all devices and flavors.
nofpu-objs-all := $(call tmpl-all,objs-tmpl,$(OBJS_SDK_NOFPU))
# SDK tag object for all devices and flavors.
sdktag-all := $(call tmpl-all,objs-tmpl,$(SDKTAG_OBJ))

# Cache all needed paths for fixpath.
$(call fixpath-cache, \
	$(call objs-fixpath-cache,$(OBJS_ALL)) \
	$(call objs-fixpath-cache,$(OBJS_ALL_DBG),$(BUILD_FLAVOR_DBG)) \
	$(lib-all) $(crt0-all))

# Add outputs to clean templates.
CLEAN_PATH_TMPL += clean-sdkdir-tmpl

# Enable secondary expansion.
.SECONDEXPANSION:

# Rule to archive prerequisite objects into a library.
$(lib-all): | $$(@D)
	$(call info-cmd,LIB)
	@$(call trace, \
		$(AR) -rc $(call fixpath-bu,$@) $(call fixpath-bu,$^))

# Build SDK objects for SDK library.
$(sdk-all): $$(call tmpl-build,objs-tmpl,$$(OBJS_SDK))

# Build Nuvoton SDK objects for Nuvoton SDK library.
$(nuvo-all): $$(call tmpl-build,objs-tmpl,$$(OBJS_NUVO))

# Rule to link crt0 objects into a partially linked object.
$(crt0-all): $$(call tmpl-build,objs-tmpl,$$(OBJS_CRT0)) | $$(@D)
	$(call info-cmd,LNK)
	@$(call trace, \
		$(LD) -r $(call fixpath-bu,$^) -o $(call fixpath-bu,$@))
# Build debug-only crt0 objects for debug builds.
$(crt0-dbg): $$(call tmpl-build,objs-tmpl,$$(OBJS_CRT0_DBG))

# Define EVICSDK_SDKTAG for the SDK tag target (asm).
$(sdktag-all): ASFLAGS += -DEVICSDK_SDKTAG=\"$(get-sdktag)\"
# Always rebuild SDK tag (.PHONY doesn't play well with pattern rules).
$(sdktag-all): .FORCE

# Build no-FPU objects with no-FPU CPU flags.
$(nofpu-objs-all): CPUFLAGS := $(CPUFLAGS_NOFPU)

# Set extra C/C++ flags for SDK/crt0 objects.
# TODO: see CCFLAGS_EXTRA.
#$(sdk-all) $(crt0-all): CFLAGS += $(CCFLAGS_EXTRA)
#$(sdk-all) $(crt0-all): CXXFLAGS += $(CCFLAGS_EXTRA)

# Device-flavor target: build all outputs.
$(devfla-all): \
	$$(call tmpl-build,crt0-tmpl) \
	$$(call tmpl-build,nuvo-tmpl) \
	$$(call tmpl-build,sdk-tmpl)

# Rule to build documentation via doxygen.
docs:
	doxygen

# Rule to clean documentation.
clean-docs:
	rm -rf $(DOCDIR)

# Set BUILD_* for output targets.
$(call build-vars-rules,sdk-tmpl)
$(call build-vars-rules,nuvo-tmpl)
$(call build-vars-rules,crt0-tmpl)

# Generate directory targets.
$(call mkdir-rules,objs-dirs-tmpl,$(OBJS_ALL) $(OBJS_ALL_DBG))
$(call mkdir-rules,sdkdir-tmpl)

# Set object directories as order-only prerequisites for object targets.
$(objs-all): | $$(@D)

.FORCE:
.PHONY: .FORCE docs
