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
 * Copyright (C) 2015 ReservedField
 */

#include <M451Series.h>
#include <Dataflash.h>

/**
 * PLL clock: 72MHz.
 */
#define PLL_CLOCK 72000000

/**
 * Initializes the system.
 * System control registers must be unlocked.
 */
void SYS_Init() {
	// HIRC clock (internal RC 22.1184MHz)
	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
	
	// HCLK clock source: HIRC, HCLK source divider: 1
	CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));
	
	// HXT clock (external XTAL 12MHz)
	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	
	// Enable 72MHz optimization
	FMC_EnableFreqOptimizeMode(FMC_FTCTL_OPTIMIZE_72MHZ);
	
	// Core clock: PLL
	CLK_SetCoreClock(PLL_CLOCK);
	CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);
	
	// SPI0 clock: PCLK0
	CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK0, MODULE_NoMsk);
	CLK_EnableModuleClock(SPI0_MODULE);
	
	// TMR0 clock: HXT
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
	CLK_EnableModuleClock(TMR0_MODULE);

	// Enable BOD (reset, 2.2V)
	SYS_EnableBOD(SYS_BODCTL_BOD_RST_EN, SYS_BODCTL_BODVL_2_2V);
	
	// Update system core clock
	SystemCoreClockUpdate();

	// Initialize dataflash
	// TODO: why is SYS_UnlockReg() needed? Should be already unlocked.
	SYS_UnlockReg();
	Dataflash_Init();
}