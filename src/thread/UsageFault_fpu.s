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
	LDR    R2, =0xE000ED28
	LDR    R3, [R2]
	TST    R3, #(1 << 19)
	BEQ    UsageFault_Handler_escalate

	@ Check if FPU is enabled (i.e. CPACR[23:20] != 0000)
	@ If it is, this is not an FPU CP denial
	LDR    R0, =0xE000ED88
	LDR    R1, [R0]
	TST    R1, #(0xF << 20)
	BNE    UsageFault_Handler_escalate

	@ Clear NOCP flag
	BIC    R3, #(1 << 19)
	STR    R3, [R2]

	@ Enable FPU (enable CP10/CP11, i.e. CPACR[23:20] = 1111)
	ORR    R1, #(0xF << 20)
	STR    R1, [R0]
	@ FPU enabled: sync barrier, flush pipeline
	DSB
	ISB

	@ If the current thread is the FPU holder, we are done
	LDR    R0, =Thread_fpuState
	LDM    R0, {R1-R2}
	TEQ    R1, R2
	IT     EQ
	BXEQ   LR

	PUSH   {R4, R5}

	@ Save FPCCR, clear LSPACT (FPCCR[0] = 0)
	LDR    R4, =0xE000EF34
	LDR    R5, [R4]
	BIC    R12, R5, #(1 << 0)
	STR    R12, [R4]

	@ Note that we never check whether we are in thread mode
	@ or in an interrupt. This means that an FPU-using ISR will
	@ behave like the thread it preempted used FPU.
	@ While this is unnecessary, it solves the race where an
	@ ISR uses the FPU before the thread does in this quantum:
	@ if we only enable FPU without switching context, there
	@ would be no UsageFault when the thread uses it, resulting
	@ in corrupted context.
	@ This can be avoided by tailchaining PendSV and disabling
	@ FPU again there, but the performance has to be evaluated.

	@ If needed, save FPU context for FPU holder
	TEQ    R1, #0
	IT     NE
	VSTMNE R1, {S0-S31}

	@ If needed, restore FPU context for current
	@ thread and set it as FPU holder
	TEQ    R2, #0
	ITT    NE
	VLDMNE R2, {S0-S31}
	STRNE  R2, [R0]

	@ Restore FPCCR
	STR    R5, [R4]

	POP    {R4, R5}

	@ If other fault flags were set, escalate
	TEQ    R3, #0
	IT     EQ
	BXEQ   LR

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
