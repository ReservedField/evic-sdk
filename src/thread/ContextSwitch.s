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
	@ we can easily context switch between threads. Since we never
	@ preempt ISRs, we always work with the PSP.
	.thumb_func

	PUSH    {LR}

	@ Call Thread_Schedule(EXC_RETURN)
	@ We can delay the context push because the ABI enforces
	@ routines to save and restore R4-R11 and S16-S31.
	@ EXC_RETURN is ignored if FPU support is disabled, save
	@ a cycle there for the no-FPU build.
	@ Return: R0 = new ctx (or NULL), R1 = old ctx (or NULL)
#ifdef EVICSDK_FPU_SUPPORT
	MOV     R0, LR
#endif
	LDR     R1, =Thread_Schedule
	BLX     R1

	POP     {LR}

	@ If needed, save old thread context
	TEQ     R1, #0
	ITT     NE
	MRSNE   R2, PSP
	STMNE   R1, {R2, R4-R11, LR}

	@ If needed, switch context
	@ Also clear any exclusive lock held by the old thread
	TEQ     R0, #0
	ITTT    NE
	LDMNE   R0, {R2, R4-R11, LR}
	MSRNE   PSP, R2
	CLREXNE

	BX      LR
