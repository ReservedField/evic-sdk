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
 */

// MemManage fault status
#define FAULT_GET_MMFSR() (SCB->CFSR & 0xFF)
// Bus fault status
#define FAULT_GET_BFSR()  ((SCB->CFSR >> 8) & 0xFF)
// Usage fault status
#define FAULT_GET_UFSR()  ((SCB->CFSR >> 16) & 0xFFFF)

/**
 * Dumps registers to display.
 * This is an internal function.
 *
 * @param yOff  Y coordinate to start putting text at.
 * @param stack Pointer to the stack where the registers
 *              have been pushed.
 */
static void Fault_DumpRegisters(uint8_t y, uint32_t *stack) {
	char buf[9];
	uint8_t i;

	for(i = 0; i < 8; i++) {
		siprintf(buf, "%08lx", stack[i]);
		Display_PutText(0, y, buf, FONT_DEJAVU_8PT);
		y += FONT_DEJAVU_8PT->height;
	}
}

/**
 * Handles an hard fault.
 * This is an internal function.
 *
 * @param stack Pointer to the stack where the registers
 *              have been pushed.
 */
 void Fault_HandleHardFault(uint32_t *stack) {
	char buf[16];
	uint8_t mmfsr, bfsr;
	uint16_t ufsr;

	mmfsr = FAULT_GET_MMFSR();
	bfsr = FAULT_GET_BFSR();
	ufsr = FAULT_GET_UFSR();

	if(!(SCB->HFSR & SCB_HFSR_FORCED_Msk)) {
		siprintf(buf, "HARD\n%08lx", SCB->HFSR);
	}
	else if(ufsr != 0) {
		siprintf(buf, "USAGE\n%04x", ufsr);
	}
	else if(bfsr != 0) {
		siprintf(buf, "BUS %02x\n%08lx", bfsr, SCB->BFAR);
	}
	else if(mmfsr != 0) {
		siprintf(buf, "MEM %02x\n%08lx", mmfsr, SCB->MMFAR);
	}
	else {
		siprintf(buf, "???");
	}

	Display_Clear();
	Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
	Fault_DumpRegisters(FONT_DEJAVU_8PT->height * 2, stack);
	Display_Update();

	while(1);
}

__attribute__((naked)) void HardFault_Handler() {
	asm volatile("@ HardFault_Handler\n\t"
		"TST   LR, #4\n\t"
		"ITE   EQ\n\t"
		"MRSEQ R0, MSP\n\t"
		"MRSNE R0, PSP\n\t"
		"LDR   R1, =Fault_HandleHardFault\n\t"
		"BX    R1"
	);
}
