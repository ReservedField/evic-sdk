@ This file is part of eVic SDK.
@
@ eVic SDK is free software: you can redistribute it and/or modify
@ it under the terms of the GNU General Public License as published by
@ the Free Software Foundation, either version 3 of the License, or
@ (at your option) any later version.
@
@ eVic SDK is distributed in the hope that it will be useful,
@ but WITHOUT ANY WARRANTY; without even the implied warranty of
@ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
@ GNU General Public License for more details.
@
@ You should have received a copy of the GNU General Public License
@ along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
@
@ Copyright (C) 2016 ReservedField

#ifndef EVICSDK_ASM_VERIFY_H
#define EVICSDK_ASM_VERIFY_H

	.syntax unified

@ Generates the verification info for a device.
@ pid  : product ID.
@ maxhw: maximum supported hardware version.
@        A.BC is passed as integer ABC.
.macro DEVICE_INFO pid maxhw
	.section .verify
	.ascii "Joyetech APROM" @ Vendor
	.ascii "\pid"           @ Product ID
	@ Maximum supported hardware version
	.byte \maxhw / 100
	.byte \maxhw % 100 / 10
	.byte \maxhw % 10
	@ Vendor string must start at least 26 bytes from end of file
	@ PID string must start at least 9 bytes from end of file
	.space 5
.endm

#endif
