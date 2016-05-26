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

.global PendSV_Handler
PendSV_Handler:
	@ This has the lowest possible priority, so it will not
	@ preempt other interrupt handlers. This means it will only
	@ preempt thread code, or will tailchain to an interrupt
	@ that preempted thread code. In other words, when we get here
	@ we can easily context switch between threads.
	.thumb_func

	@ Check Thread_curTcb
	LDR      R0, =Thread_curTcb
	LDR      R1, [R0]
	TEQ      R1, #0

	@ If Thread_curTcb != NULL, push old context to PSP
	@ Hardware push: FPSCR, S15-S0, xPSR, PC, LR, R12, R3-R0
	@ Software push: R4-R11, S16-S31
	ITTTT    NE
	MRSNE    R0, PSP
	STMFDNE  R0!, {R4-R11}
	VSTMDBNE R0!, {S16-S31}
	MSRNE    PSP, R0

	@ Clear any exclusive lock held by the old thread, memory barrier
	CLREX
	DMB

	@ Call Thread_Schedule(PSP)
	@ Returns new PSP
	LDR      R1, =Thread_Schedule
	BLX      R1

	@ Pop S16-S31, R4-R11 from new PSP and restore PSP
	VLDMIA   R0!, {S16-S31}
	LDMFD    R0!, {R4-R11}
	MSR      PSP, R0

	@ Return to thread mode, use PSP, restore FP state
	LDR      LR, =0xFFFFFFED

	@ Resume thread
	BX       LR
