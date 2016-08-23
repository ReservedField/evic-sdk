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

# If you need to, set INCDIRS(_*) *before* including this file.

ifndef __evicsdk_make_common_inc
__evicsdk_make_common_inc := 1

include $(EVICSDK)/make/gmsl/gmsl
include $(EVICSDK)/make/Helper.mk
include $(EVICSDK)/make/Device.mk

# Check make version. The sort/firstword trick fails if MAKE_VERSION is empty
# (make < 3.69), so we $(and) it with MAKE_VERSION.
$(if $(EVICSDK_MAKE_DEBUG),$(info Make version: $(MAKE_VERSION)))
ifneq ($(and $(MAKE_VERSION),$(firstword $(sort $(MAKE_VERSION) 3.81))),3.81)
$(error Make >= 3.81 required)
endif

# Disable all builtin rules (they only slow us down).
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Detect the OS, setting the following variables:
# IS_WIN_OS: 1 on Windows, empty otherwise.
# IS_CYGWIN: 1 on Windows + Cygwin, empty otherwise.
# On Windows + Cygwin both will be 1.
# On Windows + MinGW only IS_WIN_OS will be 1.
IS_WIN_OS := $(if $(findstring windows,$(call lc,$(OS))),1)
IS_CYGWIN := $(if $(findstring cygwin,$(call lc,$(shell uname -s))),1)
$(if $(EVICSDK_MAKE_DEBUG),$(info Detected OS: \
	$(if $(IS_WIN_OS),Windows ($(if $(IS_CYGWIN),Cygwin,native)),Unix-like)))

# OS null device (e.g. /dev/null on Linux and Cygwin, NUL on Windows).
NULLDEV := $(if $(and $(IS_WIN_OS),$(call not,$(IS_CYGWIN))),NUL,/dev/null)
# GCC/binutils null device (e.g. /dev/null on Linux, NUL on Cygwin, Windows).
NULLDEV_TC := $(if $(IS_WIN_OS),NUL,/dev/null)

# Detect the compiler based on CC, setting the following variables:
# CC_IS_CLANG: 1 if Clang, empty if GCC.
# If the compiler is not Clang, CC is set to arm-none-eabi-gcc.
CC_IS_CLANG := $(if $(findstring clang version,$(shell $(CC) -v 2>&1)),1)
ifndef CC_IS_CLANG
	CC := arm-none-eabi-gcc
endif
$(if $(EVICSDK_MAKE_DEBUG),$(info Detected compiler: \
	$(if $(CC_IS_CLANG),Clang,GCC)))

# Set up CPU flags for the compiler. Use EVICSDK_FPU_DISABLE to disable FPU.
# Sets the following variables:
# CPUFLAGS_GCC_NOFPU: flags for no-FPU build, GCC compiler.
# CPUFLAGS_GCC_FPU: flags for FPU build, GCC compiler.
# CPUFLAGS_GCC: flags for current build, GCC compiler.
# CPUFLAGS_NOFPU: flags for no-FPU build, detected compiler.
# CPUFLAGS_FPU: flags for FPU build, detected compiler.
# CPUFLAGS: flags for current build, detected compiler.
CPUFLAGS_GCC_NOFPU := -mcpu=cortex-m4 -mthumb
CPUFLAGS_GCC_FPU := $(CPUFLAGS_GCC_NOFPU) -mfloat-abi=hard -mfpu=fpv4-sp-d16
CPUFLAGS_GCC := \
	$(if $(EVICSDK_FPU_DISABLE),$(CPUFLAGS_GCC_NOFPU),$(CPUFLAGS_GCC_FPU))
ifndef CC_IS_CLANG
	CPUFLAGS_NOFPU := $(CPUFLAGS_GCC_NOFPU)
	CPUFLAGS_FPU := $(CPUFLAGS_GCC_FPU)
else
	CPUFLAGS_FPU := -target armv7em-none-eabi
	CPUFLAGS_NOFPU := $(CPUFLAGS_FPU) -mfpu=none
endif
CPUFLAGS := $(if $(EVICSDK_FPU_DISABLE),$(CPUFLAGS_NOFPU),$(CPUFLAGS_FPU))

# Detect library paths for the selected CPU flags, setting ARM_LIBDIRS_FIXBU.
# This is already fixed for binutils (see the fixpath-* docs in Helper.mk).
# The linker search path must follow the exact order of ARM_LIBDIRS_FIXBU.
ARM_LIBDIRS_FIXBU := $(subst $(if $(IS_WIN_OS),;,:), ,$(shell \
	arm-none-eabi-gcc $(CPUFLAGS_GCC) --print-search-dirs | \
	sed -n 's/^libraries:\s*=\?\(.*\)/\1/p'))
ifndef ARM_LIBDIRS_FIXBU
$(error Could not detect arm-none-eabi library paths)
endif
ifdef EVICSDK_MAKE_DEBUG
$(info ARM library paths (raw): $(ARM_LIBDIRS_FIXBU))
$(info ARM library paths (canonical): \
	$(call path-canon,$(call unfixpath-tc,$(ARM_LIBDIRS_FIXBU))))
endif

# Nuvoton SDK root path.
NUVOSDK := $(EVICSDK)/nuvoton-sdk/Library

# Gets GCC include directories for a language.
# Argument 1: language.
get-gcc-incdirs = $(shell \
	arm-none-eabi-gcc $(CPUFLAGS_GCC) -x $1 -v -E $(NULLDEV_TC) 2>&1 | \
	sed -n '/<\.\.\.>/,/End/p' | tail -n +2 | head -n -1 | sed 's/^\s*//')

# Clang doesn't know about standard headers for arm-none-eabi.
# Fix it by using GCC's headers. We'll add -nostdinc later.
# TODO: this works, but generates warnings. Find a better way.
ifdef CC_IS_CLANG
	__INCDIRS_GCC_C := $(call get-gcc-incdirs,c)
	__INCDIRS_GCC_CXX := $(call get-gcc-incdirs,c++)
	__INCDIRS_GCC_C_UNFIX := $(call unfixpath-tc,$(__INCDIRS_GCC_C))
	__INCDIRS_GCC_CXX_UNFIX := $(call unfixpath-tc,$(__INCDIRS_GCC_CXX))
ifdef EVICSDK_MAKE_DEBUG
$(info GCC C include paths (raw): $(__INCDIRS_GCC_C))
$(info GCC C include paths (canonical): \
	$(call path-canon,$(__INCDIRS_GCC_C_UNFIX)))
$(info GCC C++ include paths (raw): $(__INCDIRS_GCC_CXX))
$(info GCC C++ include paths (canonical): \
	$(call path-canon,$(__INCDIRS_GCC_CXX_UNFIX)))
endif
	INCDIRS_C += $(__INCDIRS_GCC_C_UNFIX)
	INCDIRS_CXX += $(__INCDIRS_GCC_CXX_UNFIX)
endif

# Add include directories for SDKs to INCDIRS.
INCDIRS += \
	$(NUVOSDK)/CMSIS/Include \
	$(NUVOSDK)/Device/Nuvoton/M451Series/Include \
	$(NUVOSDK)/StdDriver/inc \
	$(EVICSDK)/include

# Cache all needed paths for fixpath.
$(call fixpath-cache,$(INCDIRS) $(INCDIRS_C) $(INCDIRS_CXX))

# Gets the compiler flags for the specified include paths.
# Argument 1: list of include paths.
get-incflags = $(foreach d,$(call fixpath-cc,$1),-I$d)

# Set up toolchain flags. Appends to the following variables:
# CFLAGS: C compiler flags.
# CXXFLAGS: C++ compiler flags.
# ASFLAGS: assembler flags.
# Note that those are NOT inclusive of CPU flags.
__CC_FLAGS := \
	$(call get-incflags,$(INCDIRS)) \
	-Os -fdata-sections -ffunction-sections \
	$(if $(CC_IS_CLANG),-nostdinc)
ifndef EVICSDK_FPU_DISABLE
	__CC_FLAGS += -DEVICSDK_FPU_SUPPORT
	ASFLAGS += -DEVICSDK_FPU_SUPPORT
endif
__CFLAGS_INCDIRS := $(call get-incflags,$(INCDIRS_C))
__CXXFLAGS_INCDIRS := $(call get-incflags,$(INCDIRS_CXX))
CFLAGS += $(__CFLAGS_INCDIRS) $(__CC_FLAGS)
CXXFLAGS += $(__CXXFLAGS_INCDIRS) $(__CC_FLAGS) -fno-exceptions -fno-rtti
__EXTRA_FLAGS_DBG := -g

# Set up toolchain tool names. CC is already set.
# We use CC as the assembler to take advantage of preprocessing.
CXX := $(CC)
AS := $(CC)
LD := arm-none-eabi-ld
AR := arm-none-eabi-ar
OBJCOPY := arm-none-eabi-objcopy

# Default device: all devices.
EVICSDK_MAKE_DEFAULT_DEVICE ?= all
# Default flavor: all flavors.
EVICSDK_MAKE_DEFAULT_FLAVOR ?= all

# Template that wraps the template specified by the extra argument, converting
# empty arguments to "all". Doesn't support extra arguments to the template.
ea-wrap = $(call $3,$(or $1,all),$(or $2,all))
# Template that wraps the template specified by the extra argument, ignoring
# flavor and always passing in an empty one. Doesn't support extra arguments to
# the template.
dev-wrap = $(call $3,$1)
# Template that wraps the template specified by the extra argument, ignoring
# device and always passing in an empty one. Doesn't support extra arguments to
# the template.
fla-wrap = $(call $3,,$2)

# Generates phony rules for a template that default:
#  - device-all targets to all device-flavor targets for the specified device;
#  - all-flavor targets to all device-flavor targets for the specified flavor;
#  - all-all and all targets to all device-flavor targets.
# Argument 1: template name. Must produce the correct target names when called
#             with one empty argument.
# Argument 2: extra template argument (optional).
alias-devfla-rules = $(eval $(call __alias-devfla-rules,$1,$2))
define __alias-devfla-rules
$(call tmpl-rule-all-device,prereq-rule,$1,ea-wrap $1,$2)
$(call tmpl-rule-all-flavor,prereq-rule,$1,ea-wrap $1,$2)
$(call $1,all,all,$2) $(call $1,all,,$2): $(call tmpl-all,$1,$2)
.PHONY: $(call tmpl-flavor,$1,all,$2) $(call tmpl-device,$1,all,$2) \
	$(call $1,all,all,$2) $(call $1,all,,$2)
endef

# Generates phony rules for a template that default:
#  - device targets to device-flavor targets for the default flavor(s);
#  - flavor targets to device-flavor targets for the default device(s);
#  - the empty target to device-flavor targets for the default flavor(s) and
#    device(s).
# Argument 1: template name. Must produce the correct target names when called
#             with one or both empty arguments.
# Argument 2: extra template argument (optional).
alias-default-rules = $(eval $(call __alias-default-rules,$1,$2))
define __alias-default-rules
$(foreach f,$(EVICSDK_MAKE_DEFAULT_FLAVOR),$(call \
	tmpl-rule-flavor,prereq-rule,$f,$1,dev-wrap $1,$2))
$(foreach d,$(EVICSDK_MAKE_DEFAULT_DEVICE),$(call \
	tmpl-rule-device,prereq-rule,$d,$1,fla-wrap $1,$2))
$(call $1,,,$2): $(foreach f,$(EVICSDK_MAKE_DEFAULT_FLAVOR),$(foreach \
	d,$(EVICSDK_MAKE_DEFAULT_DEVICE),$(call $1,$d,$f,$2)))
.PHONY: $(call tmpl-flavor,$1,,$2) $(call tmpl-device,$1,,$2)
endef

# Helper that calls both alias-devfla-rules and alias-default-rules.
# Argument 1: template name. Must produce the correct target names when called
#             with one or both empty arguments.
# Argument 2: extra template argument (optional).
alias-rules = $(call alias-devfla-rules,$1,$2) \
	$(call alias-default-rules,$1,$2)

# SDK output directory.
SDKDIR = lib
# Base object output directory.
OBJDIR = obj

# SDK output directory template.
# Extra argument: SDK root (empty for a relative path).
sdkdir-tmpl = $(if $3,$3/)$(call dir-tmpl,$1,$2,$(SDKDIR))
# Object output directory template.
objdir-tmpl = $(call dir-tmpl,$1,$2,$(OBJDIR))
# Object output directory template for clean. Accepts empty arguments.
clean-objdir-tmpl = $(call clean-dir-tmpl,$1,$2,$(OBJDIR))
# Objects output path template. Extra argument: object list.
objs-tmpl = $(addprefix $(call objdir-tmpl,$1,$2)/,$3)
# Objects output directory template (no trailing slash, duplicates removed).
# Extra argument: object list.
objs-dirs-tmpl = $(patsubst %/,%,$(sort $(dir $(call objs-tmpl,$1,$2,$3))))
# Object pattern template.
objpat-tmpl = $(call objdir-tmpl,$1,$2)/%.o
# Device-flavor template. Accepts empty arguments.
# If both device and flavor are empty, "def" is returned.
devfla-tmpl = $(or $1$(and $1,$2,-)$2,def)
# Clean-device-flavor template. Accepts empty arguments.
clean-devfla-tmpl = clean$(if $1,-$1)$(if $2,-$2)

# Gets the paths to all possible sources for the given objects.
# Argument 1: object list.
get-srcs = $(foreach e,c cpp s,$(patsubst %.o,%.$e,$1))

# Gets object and source paths to cache for fixpath.
# Argument 1: object list.
# Argument 2: flavor (optional).
objs-fixpath-cache = $(if $2,$(call tmpl-flavor,objs-tmpl,$2,$1),$(call \
	tmpl-all,objs-tmpl,$1)) $(call get-srcs,$1)

# Object patterns for all devices, debug flavor.
objpat-dbg := $(call tmpl-flavor,objpat-tmpl,$(BUILD_FLAVOR_DBG))
# Device-flavor targets for all devices and flavors.
devfla-all := $(call tmpl-all,devfla-tmpl)
# Clean-device-flavor targets for all devices and flavors.
clean-devfla-all := $(call tmpl-all,clean-devfla-tmpl)
# Clean-device-all targets for all devices.
clean-devall-all := $(call tmpl-flavor,clean-devfla-tmpl,all)
# Clean-all-flavor targets for all flavors.
clean-allfla-all := $(call tmpl-device,clean-devfla-tmpl,all)
# Clean-all-all and clean-all targets.
clean-all := $(call clean-devfla-tmpl,all,all) $(call clean-devfla-tmpl,all)

# Templates for paths to be removed on clean. Each template must support being
# called with one or both empty arguments and can return multiple paths.
# Add objects to clean templates.
CLEAN_PATH_TMPL += clean-objdir-tmpl

# Object compilation rules macro.
# Since those are pattern rules, make assumes all targets are built within a
# single invocation when multiple targets are present. Because of this we need
# to redefine the rules for each device and flavor instead of writing them once
# with $(call tmpl-all,objpat-tmpl) as target.
define compile-rules
$3: %.c
	$$(call info-cmd,CC)
	@$$(call trace, \
		$$(CC) $$(CPUFLAGS) $$(CFLAGS) \
			-c $$(call fixpath-cc,$$<) -o $$(call fixpath-cc,$$@))
$3: %.cpp
	$$(call info-cmd,CXX)
	@$$(call trace, \
		$$(CXX) $$(CPUFLAGS) $$(CXXFLAGS) \
			-c $$(call fixpath-cc,$$<) -o $$(call fixpath-cc,$$@))
$3: %.s
	$$(call info-cmd,AS)
	@$$(call trace, \
		$$(AS) $$(CPUFLAGS) $$(ASFLAGS) -c -x assembler-with-cpp \
			$$(call fixpath-cc,$$<) -o $$(call fixpath-cc,$$@))
endef

# Compilation rules for all object targets.
$(call tmpl-rule-all,compile-rules,objpat-tmpl)

# Set target-specific variables for debug targets.
$(objpat-dbg): CFLAGS += $(__EXTRA_FLAGS_DBG)
$(objpat-dbg): CXXFLAGS += $(__EXTRA_FLAGS_DBG)
$(objpat-dbg): ASFLAGS += $(__EXTRA_FLAGS_DBG)

# Common rule for all clean targets.
$(clean-devfla-all) $(clean-devall-all) $(clean-allfla-all) $(clean-all):
	rm -rf $(call tmpl-cat,$(CLEAN_PATH_TMPL),$(BUILD_DEVICE),$(BUILD_FLAVOR))

# All aliases for device-flavor targets.
$(call alias-rules,devfla-tmpl)
# Default aliases for clean targets.
$(call alias-default-rules,clean-devfla-tmpl)

# Set BUILD_* for object, device-flavor and clean-device-flavor targets.
# We don't have to split up object patterns for target-specific variables.
$(call build-vars-rules,objpat-tmpl)
$(call build-vars-rules,devfla-tmpl)
$(call build-vars-rules,clean-devfla-tmpl)
# Set BUILD_* for clean-device-all targets (empty flavor).
$(call tmpl-rule-flavor,build_device_flavor-rules,,ea-wrap,,clean-devfla-tmpl)
# Set BUILD_* for clean-all-flavor targets (empty device).
$(call tmpl-rule-device,build_device_flavor-rules,,ea-wrap,,clean-devfla-tmpl)

# Default target: empty device-flavor target.
.DEFAULT_GOAL := $(call devfla-tmpl)

.PHONY: $(devfla-all) \
	$(clean-devfla-all) $(clean-devall-all) $(clean-allfla-all) $(clean-all)

endif # __evicsdk_make_common_inc
