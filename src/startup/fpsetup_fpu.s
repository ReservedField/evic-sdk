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

.syntax unified

.global Startup_FpSetup
.thumb_func
Startup_FpSetup:
	@ Enable lazy stacking
	@ (ASPEN = 1, LSPEN = 1, i.e. FPCCR[31:30] = 11)
	LDR R0, =0xE000EF34
	LDR R1, [R0]
	ORR R1, R1, #(0x3 << 30)
	STR R1, [R0]

	@ Enable FPU (enable CP10/CP11, i.e. CPACR[23:20] = 1111)
	LDR R0, =0xE000ED88
	LDR R1, [R0]
	ORR R1, R1, #(0xF << 20)
	STR R1, [R0]

	@ FPU enabled: sync barrier, flush pipeline
	DSB
	ISB

	BX  LR
