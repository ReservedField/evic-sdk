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

.global UsageFault_Handler
UsageFault_Handler:
	.thumb_func

	@ Check if NOCP (CFSR[16+3]) is set
	@ If not, this is not a CP denial
	LDR    R0, =0xE000ED28
	LDR    R1, [R0]
	TST    R1, #(1 << 19)
	BEQ    UsageFault_Handler_escalate

	@ Check if FPU is enabled (i.e. CPACR[23:20] != 0000)
	@ If it is, this is not an FPU CP denial
	LDR    R0, =0xE000ED88
	LDR    R1, [R0]
	TST    R1, #(0xF << 20)
	BNE    UsageFault_Handler_escalate

	@ Enable FPU (enable CP10/CP11, i.e. CPACR[23:20] = 1111)
	ORR    R1, #(0xF << 20)
	STR    R1, [R0]
	@ FPU enabled: sync barrier, flush pipeline
	DSB
	ISB

	@ If the current thread is the FPU holder, we are done
	LDR    R0, =Thread_fpuHolderCtx
	LDR    R2, [R0]
	LDR    R1, =Thread_fpuCurCtx
	LDR    R3, [R1]
	TEQ    R2, R3
	IT     EQ
	BXEQ   LR

	@ If needed, save FPU context for FPU holder
	TEQ    R2, #0
	IT     NE
	VSTMNE R2, {S16-S31}

	@ If needed, restore FPU context for current
	@ thread and set it as FPU holder
	TEQ    R3, #0
	ITT    NE
	VLDMNE R3, {S16-S31}
	STRNE  R3, [R0]

	BX     LR

UsageFault_Handler_escalate:
	@ If this is not an FPU CP denial, we want to escalate
	@ it to a hard fault to hand it over to the SDK fault
	@ handler (if enabled). We do this by disabling the
	@ UsageFault exception (i.e. SHCSR[18] = 0) and returning.
	@ The faulting instruction will be executed again, and
	@ this time it will escalate to a hard fault because the
	@ UsageFault handler is disabled. If the SDK fault handler
	@ is enabled, it will re-enable the UsageFault handler for
	@ us. If not, the system will halt.
	LDR    R0, =0xE000ED24
	LDR    R1, [R0]
	BIC    R1, #(1 << 18)
	STR    R1, [R0]
	BX     LR
