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

ifndef __evicsdk_make_helper_inc
__evicsdk_make_helper_inc := 1

include $(EVICSDK)/make/gmsl/gmsl
include $(EVICSDK)/make/Device.mk

# Terms used in documentation:
# OPTIONAL: an optional argument can be empty. Make syntax allows you to omit
#           empty arguments if they are the last ones.
# TEMPLATE: an user-defined function that generates some kind of string from
#           the following arguments:
#           Argument 1: device name.
#           Argument 2: flavor.
#           Argument 3: extra argument (if specified for tmpl-*).
# RULE MACRO: an user-defined function that will be eval'ed (escape your $s!).
#             Called a rule macro because it's usually used to define rules.
#             It takes the following arguments:
#             Argument 1: device name.
#             Argument 2: flavor.
#             Argument 3: template output (if a template was specified for
#                         tmpl-rule-*).
#             Argument 4: extra argument (if specified for tmpl-rule-*).

# Build flavors.
BUILD_FLAVOR_DBG := dbg
BUILD_FLAVOR_REL := rel
BUILD_FLAVORS := $(BUILD_FLAVOR_DBG) $(BUILD_FLAVOR_REL)

# Converts a list of Cygwin style paths to native Windows style.
# Argument 1: list of paths to convert.
cygpath-w = $(if $(strip $1),$(shell cygpath -w $(strip $1)))

# Converts a list of native Windows style paths to Cygwin style.
# Argument 1: list of paths to convert.
cygpath-u = $(if $(strip $1),$(shell cygpath -u $(strip $1)))

# Converts a list of paths to the needed system style for binutils (BU) or the
# C compiler (CC). A few assumptions need to be made for Windows + Cygwin, due
# to path style differences between native and Cygwin binaries. We assume
# arm-none-eabi-gcc and binutils are native binaries, while Clang is a Cygwin
# binary. This is consistent with the available packages. This means we'll
# fixup paths through cygpath-w when passing them to GCC/binutils on Cygwin,
# but not when passing them to Clang.
# IS_CYGWIN and CC_IS_CLANG must be set prior to calling those. Since the
# conversion is expensive, paths are internally cached. They can be manually
# cached through fixpath-cache, which expands to empty.
# Argument 1: list of paths to fix.
fixpath-bu = $(if $(IS_CYGWIN),$(call memoize-list,cygpath-w,$1),$1)
fixpath-cc = $(if $(and $(IS_CYGWIN),$(call not,$(CC_IS_CLANG))),$(call \
	memoize-list,cygpath-w,$1),$1)
fixpath-cache = $(if $(IS_CYGWIN),$(call memoize-list-update,cygpath-w,$1))

# Converts a list of paths from GCC/binutils to shell style. Same assumptions
# are made as for fixpath-*. The paths are not internally cached (Windows uses
# colons in paths, which are not supported by memoize-list).
# Argument 1: list of paths to unfix.
unfixpath-tc = $(if $(IS_CYGWIN),$(call cygpath-u,$1),$1)

# Canonicalizes a list of paths.
# If a path doesn't exist it's retuned unchanged and surrounded by ().
# Note: $(realpath) is broken for absolute paths on Windows make 3.81. Cygwin
# make is fine (Unix-style paths). This is only used for debugging anyway.
# Argument 1: list of paths to canonicalize.
path-canon = $(foreach p,$1,$(or $(realpath $p),($p)))

# Prints a message formatted like:
# [device-flavor] command target
# This function is to be used only inside rules with BUILD_* variables set.
# Argument 1: command name (at most 3 characters).
info-cmd = $(info [$(BUILD_DEVICE)-$(BUILD_FLAVOR)] $(call \
	substr,$1   ,1,3) $@)

# Prints a string if EVICSDK_MAKE_DEBUG is defined.
# Always evaluates to the provided string.
# This is meant to be used for tracing recipe commands. Using the --trace
# option of make >= 4.0 is simpler, but we support make >= 3.81.
# Argument 1: string to trace and return.
trace = $(if $(EVICSDK_MAKE_DEBUG),$(info $(strip $1)))$(strip $1)

# Generates template output for all flavors for the specified device.
# Argument 1: template name.
# Argument 2: device name (passed as-is to template, can be anything).
# Argument 3: extra template argument (optional).
tmpl-device = $(foreach f,$(BUILD_FLAVORS),$(call $1,$2,$f,$3))

# Generates template output for all devices for the specified flavor.
# Argument 1: template name.
# Argument 2: flavor (passed as-is to template, can be anything).
# Argument 3: extra template argument (optional).
tmpl-flavor = $(foreach d,$(DEVICES),$(call $1,$d,$2,$3))

# Generates template output for all devices and flavors.
# Argument 1: template name.
# Argument 2: extra template argument (optional).
tmpl-all = $(foreach f,$(BUILD_FLAVORS),$(call tmpl-flavor,$1,$f,$2))

# Generates template output based on BUILD_* variables.
# Argument 1: template name.
# Argument 2: extra template argument (optional).
tmpl-build = $(call $1,$(BUILD_DEVICE),$(BUILD_FLAVOR),$2)

# Generates rules for all flavors for the specified device.
# Argument 1: rule macro name.
# Argument 2: device name (passed as-is to template and rule, can be anything).
# Argument 3: template name (optional).
# Argument 4: extra rule argument (optional).
# Argument 5: extra template argument (optional).
tmpl-rule-device = $(foreach f,$(BUILD_FLAVORS),$(eval $(call $1,$2,$f,$(call \
	$3,$2,$f,$5),$4)))

# Generates rules for all devices for the specified flavor.
# Argument 1: rule macro name.
# Argument 2: flavor (passed as-is to template and rule, can be anything).
# Argument 3: template name (optional).
# Argument 4: extra rule argument (optional).
# Argument 5: extra template argument (optional).
tmpl-rule-flavor = $(foreach d,$(DEVICES),$(eval $(call $1,$d,$2,$(call \
	$3,$d,$2,$5),$4)))

# Generates rules for all devices and flavors.
# Argument 1: rule macro name.
# Argument 2: template name (optional).
# ARgument 3: extra rule argument (optional).
# Argument 4: extra template argument (optional).
tmpl-rule-all = $(foreach f,$(BUILD_FLAVORS),$(call \
	tmpl-rule-flavor,$1,$f,$2,$3,$4))

# Generates rules for all devices and flavors, grouping them by device. This
# means that if a template is specified, the template output passed to the rule
# macro will be the output of tmpl-device. Since multiple flavors will be in
# the template output, the rule will be passed an empty flavor argument.
# Argument 1: rule macro name.
# Argument 2: template name (optional).
# Argument 3: extra rule argument (optional).
# Argument 4: extra template argument (optional).
tmpl-rule-all-device = $(foreach d,$(DEVICES),$(eval $(call $1,$d,,$(call \
	tmpl-device,$2,$d,$4),$3)))

# Generates rules for all devices and flavors, grouping them by flavor. This
# means that if a template is specified, the template output passed to the rule
# macro will be the output of tmpl-flavor. Since multiple devices will be in
# the template output, the rule will be passed an empty device argument.
# Argument 1: rule macro name.
# Argument 2: template name (optional).
# Argument 3: extra rule argument (optional).
# Argument 4: extra template argument (optional).
tmpl-rule-all-flavor = $(foreach f,$(BUILD_FLAVORS),$(eval $(call \
	$1,,$f,$(call tmpl-flavor,$2,$f,$4),$3)))

# Makes a list out of the outputs of a list of templates.
# Argument 1: list of template names.
# Argument 2: device name (passed as-is to template, can be anything).
# Argument 3: flavor (passed as-is to template, can be anything).
tmpl-cat = $(foreach t,$1,$(call $t,$2,$3))

# Memoizes a function that accepts and returns lists. The order of the list
# returned by the function is assumed to match the input list. There will be
# a single function call, but each input-output value pair will be cached
# separately. Already seen input values will be satisfied by the cache and
# filtered out from the function input. Returns the output list.
# Argument 1: function name.
# Argument 2: input list (colons are not allowed).
memoize-list = $(call memoize-list-update,$1,$(filter-out $(call \
	keys,__memoize-map-$1),$2))$(foreach k,$2,$(call get,__memoize-map-$1,$k))

# Caches values for memoize-list. The function semantics are the same, but the
# input-output pairs will always be forcefully cached. Expands to empty.
# Argument 1: function name.
# Argument 2: input list (colons are not allowed).
memoize-list-update = $(if $(strip $2),$(eval __memoize-$1 = $$(call \
	set,__memoize-map-$1,$$1,$$2))$(call \
	pairmap,__memoize-$1,$2,$(call $1,$2)))

# Rule macro to set BUILD_DEVICE and BUILD_FLAVOR for the targets specified by
# the template output.
# Note: this could be split into two rules and used with tmpl-rule-all-*. While
# it looks cleaner, it takes two calls (i.e. two times the same device-flavor
# loop) and make will split them up into a single target per rule internally,
# so there's no performance gain. It also becomes tricky for clean targets.
define build_device_flavor-rules
$3: BUILD_DEVICE := $1
$3: BUILD_FLAVOR := $2
endef

# Generates rules to set BUILD_DEVICE and BUILD_FLAVOR
# for the targets generated by the specified template.
# Argument 1: template name.
# Argument 2: extra template argument (optional).
build-vars-rules = $(call tmpl-rule-all,build_device_flavor-rules,$1,,$2)

# Rule macro to create the target directories specified
# by the template output.
define mkdir-target-rule
$3:
	@mkdir -p $$@
endef

# Generates rules to create the directories generated
# by the specified template.
# Argument 1: template name.
# Argument 2: extra template argument (optional).
mkdir-rules = $(call tmpl-rule-all,mkdir-target-rule,$1,,$2)

# Rule macro to set the template output as prerequisite for the target
# generated by the template specified by the extra rule argument. You can
# specify an extra argument to the template by separating it with a space
# from the template name in the extra rule argument.
define prereq-rule
$(call $(word 1,$4),$1,$2,$(word 2,$4)): $3
endef

# Output directory template (no trailing slash).
# Extra argument: base directory.
dir-tmpl = $3/$2/$1

# Output directory clean template (no trailing slash).
# Extra argument: base directory.
# Accepts one or both empty arguments. May return a list.
clean-dir-tmpl = $(if $(and $1,$(call not,$2)),$(call \
	tmpl-device,clean-dir-tmpl,$1,$3),$3$(if $2,/$2$(if $1,/$1)))

endif # __evicsdk_make_helper_inc
