/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

#include <stdio.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <Button.h>

/**
 * Hard fault:
 *   HARD
 *   <HFSR>
 * Usage fault:
 *   USAGE
 *   <UFSR>
 * Bus fault:
 *   BUS <BFSR>
 *   <BFAR>
 * Memory management fault:
 *   MEM <MMFSR>
 *   <MMFAR>
 * For all faults, register dump:
 *   <R0>
 *   <R1>
 *   <R2>
 *   <R3>
 *   <R12>
 *   <LR>
 *   <PC>
 *   <PSR>
 * Pressing left/right, high register dump:
 *   <R4>
 *   <R5>
 *   <R6>
 *   <R7>
 *   <R8>
 *   <R9>
 *   <R10>
 *   <R11>
 * Pressing fire resumes execution (only guaranteed
 * for UDF instruction).
 */

// MemManage fault status
#define FAULT_GET_MMFSR() (SCB->CFSR & 0xFF)
// Bus fault status
#define FAULT_GET_BFSR()  ((SCB->CFSR >> 8) & 0xFF)
// Usage fault status
#define FAULT_GET_UFSR()  ((SCB->CFSR >> 16) & 0xFFFF)
// Reset fault status
#define FAULT_RESET_CFSR() do { SCB->CFSR = 0; } while(0)

/**
 * Gets the button state, like Button_GetState().
 * This is needed because the normal Button library
 * is interrupt-driven, and we're blocking inside
 * a higher priority interrupt.
 * This is an internal function.
 *
 * @return Button state.
 */
static uint8_t Fault_GetButtonState() {
	uint8_t state;

	state  = PE0 ? 0 : BUTTON_MASK_FIRE;
	state |= PD2 ? 0 : BUTTON_MASK_RIGHT;
	state |= PD3 ? 0 : BUTTON_MASK_LEFT;

	return state;
}

/**
 * Dumps registers to display.
 * This is an internal function.
 *
 * @param y      Y coordinate to start putting text at.
 * @param stack  Pointer to the stack where the registers
 *               have been pushed.
 * @param isHigh True to print R4-R11, false to print the
 *               low registers.
 */
static void Fault_DumpRegisters(uint8_t y, uint32_t *stack, uint8_t isHigh) {
	char buf[9];
	int8_t i;

	for(i = 0; i < 8; i++) {
		// XXX I have no idea why I need to start from -1 for high regs
		siprintf(buf, "%08lx", stack[isHigh ? i - 1 : 8 + i]);
		Display_PutText(0, y, buf, FONT_DEJAVU_8PT);
		y += FONT_DEJAVU_8PT->height;
	}
}

/**
 * Dump fault info to display, along with the low registers.
 * This is an internal function.
 *
 * @param stack Pointer to the stack where the registers
 *              have been pushed.
 */
static void Fault_DumpFaultLow(uint32_t *stack) {
	char buf[16];
	uint8_t mmfsr, bfsr;
	uint16_t ufsr;

	mmfsr = FAULT_GET_MMFSR();
	bfsr = FAULT_GET_BFSR();
	ufsr = FAULT_GET_UFSR();

	if(!(SCB->HFSR & SCB_HFSR_FORCED_Msk)) {
		siprintf(buf, "HARD\n%08lx", SCB->HFSR);
	}
	else if(bfsr != 0) {
		siprintf(buf, "BUS %02x\n%08lx", bfsr, SCB->BFAR);
	}
	else if(mmfsr != 0) {
		siprintf(buf, "MEM %02x\n%08lx", mmfsr, SCB->MMFAR);
	}
	else if(ufsr != 0) {
		siprintf(buf, "USAGE\n%04x", ufsr);
	}
	else {
		siprintf(buf, "???");
	}

	Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
	Fault_DumpRegisters(FONT_DEJAVU_8PT->height * 2, stack, 0);
}

/**
 * Generates a pure busy CPU delay,
 * without using SysTick or timers.
 *
 * @param delay Delay (in milliseconds).
 */
static void Fault_Delay(uint16_t delay) {
	uint32_t counter;

	// 1 iteration = 3 clock cycles (@ 72MHz)
	// The non-loop overhead is negligible
	counter = delay * 24000;
	asm volatile("Fault_Delay_loop%=:\n\t"
		"SUBS  %0, #1\n\t"
		"BNE   Fault_Delay_loop%="
	: "+r" (counter));
}

/**
 * Handles a hard fault.
 * This is an internal function.
 *
 * @param stack Pointer to the stack where the registers
 *              have been pushed.
 */
 void Fault_HandleHardFault(uint32_t *stack) {
	uint8_t isHigh, btnState, oldBtnState, pressRelease;

#ifdef EVICSDK_FPU_SUPPORT
	// Re-enable UsageFault for the thread library
	// in case this is an escalated usage fault
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
#endif

	Display_Clear();
	Fault_DumpFaultLow(stack);
	Display_Update();

	isHigh = 0;
	oldBtnState = BUTTON_MASK_NONE;
	while(1) {
		btnState = Fault_GetButtonState();
		pressRelease = (btnState ^ oldBtnState) & oldBtnState;

		if(pressRelease & BUTTON_MASK_FIRE) {
			// Advance PC to next instruction
			// This only works for 16-bit Thumb, but it's ok since
			// we only guarantee it to work for UDF
			stack[14] += 2;
			break;
		}
		if(pressRelease & BUTTON_MASK_RIGHT || pressRelease & BUTTON_MASK_LEFT) {
			isHigh ^= 1;
			Display_Clear();
			if(isHigh) {
				Fault_DumpRegisters(0, stack, 1);
			}
			else {
				Fault_DumpFaultLow(stack);
			}
			Display_Update();
		}

		if(btnState != oldBtnState) {
			// Debounce
			Fault_Delay(30);
			oldBtnState = btnState;
		}
	}

	// Some more debouncing to avoid FIRE continuing
	// the next fault, too
	Fault_Delay(100);

	FAULT_RESET_CFSR();
}

__attribute__((naked)) void HardFault_Handler() {
	asm volatile("@ HardFault_Handler\n\t"
		"TST   LR, #4\n\t"
		"ITE   EQ\n\t"
		"MRSEQ R0, MSP\n\t"
		"MRSNE R0, PSP\n\t"
		"STMFD R0!, {R4-R11}\n\t"
		"LDR   R1, =Fault_HandleHardFault\n\t"
		"BX    R1"
	);
}
