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

ifndef __evicsdk_make_device_vtwom_inc
__evicsdk_make_device_vtwom_inc := 1

include $(EVICSDK)/make/Device.mk

$(call add-device,vtwom, \
	device/vtwom/src/Device.o, \
	device/vtwom/src/startup.o)

endif # __evicsdk_make_device_vtwom_inc