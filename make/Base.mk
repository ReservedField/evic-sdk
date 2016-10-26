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

# Supported variables:
# TARGET: output target name.
# OBJS: objects to compile.
# INCDIRS: include directories.
# INCDIRS_C: include directories (C only).
# INCDIRS_CXX: include directories (C++ only).
# LIBDIRS: library directories.
# CFLAGS: C compiler flags.
# CXXFLAGS: C++ compiler flags.
# ASFLAGS: assembler flags.
# LDFLAGS: linker flags.

ifndef __evicsdk_make_base_inc
__evicsdk_make_base_inc := 1

include $(EVICSDK)/make/Helper.mk
include $(EVICSDK)/make/Common.mk

# Binary output directory.
BINDIR := bin

# Linker script.
LDSCRIPT := $(EVICSDK)/linker/linker.ld

# Binary output directory template.
bindir-tmpl = $(call dir-tmpl,$1,$2,$(BINDIR))
# Binary output directory clean template.
clean-bindir-tmpl = $(call clean-dir-tmpl,$1,$2,$(BINDIR))
# ELF output path template.
elf-tmpl = $(call bindir-tmpl,$1,$2)/$(TARGET).elf
# Binary output path template. Extra argument: binary name.
bin-tmpl = $(call bindir-tmpl,$1,$2)/$3.bin
# Unecrypted binary output path template.
bin-dec-tmpl = $(call bin-tmpl,$1,$2,$(TARGET)_dec)
# Encrypted binary output path template.
bin-enc-tmpl = $(call bin-tmpl,$1,$2,$(TARGET))
# Linker map output path template.
ldmap-tmpl = $(call bindir-tmpl,$1,$2)/$(TARGET).map

# Object targets for all devices and flavors.
objs-all := $(call tmpl-all,objs-tmpl,$(OBJS))
# ELF targets for all devices and flavors.
elf-all := $(call tmpl-all,elf-tmpl)
# ELF targets for all devices, debug flavor.
elf-dbg := $(call tmpl-flavor,elf-tmpl,$(BUILD_FLAVOR_DBG))
# ELF targets for all devices, release flavor.
elf-rel := $(call tmpl-flavor,elf-tmpl,$(BUILD_FLAVOR_REL))
# Unencrypted binary targets for all devices and flavors.
bin-dec-all := $(call tmpl-all,bin-dec-tmpl)
# Unencrypted binary targets for all devices, release flavor.
bin-dec-rel := $(call tmpl-flavor,bin-dec-tmpl,$(BUILD_FLAVOR_REL))
# Encrypted binary targets for all devices and flavors.
bin-enc-all := $(call tmpl-all,bin-enc-tmpl)
# Linker map output paths for all device, debug flavor.
ldmap-dbg := $(call tmpl-flavor,ldmap-tmpl,$(BUILD_FLAVOR_DBG))
# SDK output directories for all devices and flavors.
sdk-all := $(call tmpl-all,sdkdir-tmpl,$(EVICSDK))

# Cache all needed paths for fixpath.
$(call fixpath-cache, \
	$(call objs-fixpath-cache,$(OBJS)) \
	$(elf-all) $(bin-dec-all) $(bin-enc-all) $(ldmap-dbg) $(sdk-all) \
	$(LIBDIRS) $(LDSCRIPT))

# Set up linker flags.
# We're linking with --no-warn-mismatch because the thread library is compiled
# without FPU support to avoid issues with FPU context switching, which would
# normally result in a linker error due to different FP ABIs (soft/hard). Since
# no function in the thread library accepts FP arguments or calls FP library
# functions, they will work fine together. This may trainwreck if SDK and APROM
# are compiled with different FP ABIs, but no-FPU builds are just a rarely used
# debugging tool, so it's good enough.
LDFLAGS += \
	-T$(call fixpath-bu,$(LDSCRIPT)) \
	-nostdlib -nostartfiles --gc-sections --no-warn-mismatch
# We keep -L flags separated because we want to specify them before LDFLAGS so
# that -l flags work correctly. Also, we're going to add the SDK library path
# to this on a device/flavor basis.
LDFLAGS_LIBDIRS := \
	$(foreach d,$(ARM_LIBDIRS_FIXBU) $(call fixpath-bu,$(LIBDIRS)),-L$d)

# Objcopy flags for converting ELF to unencrypted binary.
OBJCOPYFLAGS += -O binary -j .text -j .data

# Add binaries to clean templates.
CLEAN_PATH_TMPL += clean-bindir-tmpl

# Enable secondary expansion.
.SECONDEXPANSION:

# Include source dependency files.
-include $(call get-deps,$(objs-all))

# Set BUILD_* for all our targets.
$(call build-vars-rules,elf-tmpl)
$(call build-vars-rules,bin-dec-tmpl)
$(call build-vars-rules,bin-enc-tmpl)

# Rule to link objects into an ELF.
$(elf-all): $$(call tmpl-build,sdkdir-tmpl,$(EVICSDK)) \
            $$(call tmpl-build,objs-tmpl,$$(OBJS)) | $$(@D)
	$(call info-cmd,LD)
	@$(call trace, \
		$(LD) $(call fixpath-bu,$(wordlist 2,$(words $^),$^)) \
		$(LDFLAGS_LIBDIRS) $(LDFLAGS) -o $(call fixpath-bu,$@))

# Add the SDK to LDFLAGS_LIBDIRS for all ELF targets.
$(elf-all): LDFLAGS_LIBDIRS += \
	-L$(call fixpath-bu,$(call tmpl-build,sdkdir-tmpl,$(EVICSDK)))

# Generate a linker map for debug ELF targets.
$(elf-dbg): LDFLAGS += -Map=$(call fixpath-bu,$(call tmpl-build,ldmap-tmpl))

# Rule to generate an unencrypted binary from the ELF.
$(bin-dec-all): $$(call tmpl-build,elf-tmpl) | $$(@D)
	$(call info-cmd,BIN)
	@$(call trace, \
		$(OBJCOPY) $(OBJCOPYFLAGS) $(call fixpath-bu,$<) $(call fixpath-bu,$@))

# Rule to generate an encrypted binary from the unencrypted one.
$(bin-enc-all): $$(call tmpl-build,bin-dec-tmpl) | $$(@D)
	$(call info-cmd,ENC)
# Silence evic-convert, errors will still get through.
	@$(call trace, \
		evic-convert $< -o $@ > $(NULLDEV))

# Device-flavor target rule: build encrypted binary.
$(devfla-all): $$(call tmpl-build,bin-enc-tmpl)

# Set object directories as order-only prerequisites for object targets.
$(objs-all): | $$(@D)

# Generate directory targets.
$(call mkdir-rules,objs-dirs-tmpl,$(OBJS))
$(call mkdir-rules,bindir-tmpl)

# If this is remade the SDK output directory doesn't exist.
$(sdk-all):
	$(error No SDK found for $(BUILD_DEVICE)-$(BUILD_FLAVOR))

# Mark all release ELFs and unencrypted binaries as intermediate files.
.INTERMEDIATE: $(elf-rel) $(bin-dec-rel)

endif # __evicsdk_make_base_inc
