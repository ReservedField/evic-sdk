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

ifndef __evicsdk_make_device_inc
__evicsdk_make_device_inc := 1

# Supported devices list. Will be filled in by add-device.
DEVICES :=

# Registers a new device.
# Argument 1: device name.
# Argument 2: object list.
# Argument 3: crt0 object list.
add-device = $(eval DEVICES += $1)$(eval \
	__device-objs-$(strip $1) := $2)$(eval \
	__device-objs-crt0-$(strip $1) := $3)

# Extra device objects template. Flavor is ignored.
devobjs-tmpl = $(__device-objs-$1)
# Extra device crt0 objects template. Flavor is ignored.
devobjs-crt0-tmpl = $(__device-objs-crt0-$1)
# Device include path template. Flavor is ignored.
devincs-tmpl = $(EVICSDK)/device/$1/include

include $(wildcard $(EVICSDK)/device/*/Device.mk)

endif # __evicsdk_make_device_inc
