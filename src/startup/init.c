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
 * Copyright (C) 2015-2016 ReservedField
 */

#include <M451Series.h>
#include <SysInfo.h>
#include <Dataflash.h>
#include <Display.h>
#include <Battery.h>
#include <Button.h>
#include <ADC.h>
#include <Atomizer.h>
#include <Thread.h>

/**
 * PLL clock: 72MHz.
 */
#define PLL_CLOCK 72000000

/**
 * Initializes the system.
 * System control registers must be unlocked.
 */
void Sys_Init() {
	// HIRC clock (internal RC 22.1184MHz)
	CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

	// LIRC clock (internal RC 10kHz)
	CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

	// HCLK clock source: HIRC, HCLK source divider: 1
	CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

	// HXT clock (external XTAL 12MHz)
	CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

	// Enable 72MHz optimization
	FMC_EnableFreqOptimizeMode(FMC_FTCTL_OPTIMIZE_72MHZ);

	// Core clock: PLL @ 144MHz, HCLK @ 72MHz
	CLK_SetCoreClock(PLL_CLOCK);
	CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

	// SPI0 clock: PCLK0
	CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK0, 0);
	CLK_EnableModuleClock(SPI0_MODULE);

	// TMR0-3 clock: HXT
	CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_HXT, 0);
	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HXT, 0);
	CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_HXT, 0);
	CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_HXT, 0);
	CLK_EnableModuleClock(TMR0_MODULE);
	CLK_EnableModuleClock(TMR1_MODULE);
	CLK_EnableModuleClock(TMR2_MODULE);
	CLK_EnableModuleClock(TMR3_MODULE);

	// PWM clock: PLL
	CLK_SetModuleClock(PWM0_MODULE, CLK_CLKSEL2_PWM0SEL_PLL, 0);
	CLK_EnableModuleClock(PWM0_MODULE);

	// USBD clock
	CLK_SetModuleClock(USBD_MODULE, 0, CLK_CLKDIV0_USB(3));
	CLK_EnableModuleClock(USBD_MODULE);

	// Enable USB 3.3V LDO
	SYS->USBPHY = SYS_USBPHY_LDO33EN_Msk;

	// EADC clock: 72Mhz / 8
	CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(8));
	CLK_EnableModuleClock(EADC_MODULE);

	// Enable BOD (reset, 2.2V)
	SYS_EnableBOD(SYS_BODCTL_BOD_RST_EN, SYS_BODCTL_BODVL_2_2V);

	// Update system core clock
	SystemCoreClockUpdate();

	// Initialize system info and dataflash
	SysInfo_Init();
	Dataflash_Init();

	// Setup debounce, used both by buttons and battery presence pin.
	// Good buttons only need 100us, while at least 200us is needed by
	// battery presence. The original firmware uses 100ms for battery and
	// timer-based debounce for buttons, but that's excessive.
	// Older buttons need more debouncing (a few ms). Button debounce
	// isn't really noticeable up to 128 LIRC clock cycles.
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_32);

	// Initialize I/O
	Display_SetupSPI();
	Battery_Init();
	Button_Init();
	ADC_Init();
	Atomizer_Init();

	// Initialize display
	Display_Init();

	// Initialize thread manager
	Thread_Init();
}
