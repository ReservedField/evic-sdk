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

#include <M451Series.h>
#include <System.h>

/**
 * Wakeup sources (OR of SYS_WAKEUP_*) set by user.
 * By default enable any source.
 */
static volatile uint8_t Sys_wakeupSource = 0xFF;

/**
 * Source (SYS_WAKEUP_*) for last wakeup.
 */
static volatile uint8_t Sys_lastWakeupSource = 0;

/**
 * Struct to hold info about GPIO wakeup.
 */
typedef struct {
	/**< GPIO port. */
	GPIO_T *port;
	/**< Pin number. */
	uint8_t pin;
	/**< Mask for wakeup source. */
	uint8_t wakeupMask;
} Sys_WakeupGPIO_t;

/**
 * GPIO wakeup info lookup table.
 */
static const Sys_WakeupGPIO_t Sys_wakeupGPIO[4] = {
	{PE, 0, SYS_WAKEUP_FIRE},
	{PD, 2, SYS_WAKEUP_RIGHT},
	{PD, 3, SYS_WAKEUP_LEFT},
	{PD, 7, SYS_WAKEUP_BATTERY}
};

/**
 * Wakeup interrupt handler.
 */
void PWRWU_IRQHandler() {
	uint8_t i;
	const Sys_WakeupGPIO_t *gpio;

	if(!(CLK->PWRCTL & CLK_PWRCTL_PDWKIF_Msk)) {
		// Should never happen
		return;
	}

	// Set wakeup source
	for(i = 0; i < 4; i++) {
		gpio = &Sys_wakeupGPIO[i];
		if(GPIO_GET_INT_FLAG(gpio->port, 1 << gpio->pin) && (Sys_wakeupSource & gpio->wakeupMask)) {
			Sys_lastWakeupSource = gpio->wakeupMask;
			break;
		}
	}

	// Clear wakeup interrupt flag
	CLK->PWRCTL |= CLK_PWRCTL_PDWKIF_Msk;
}

void Sys_Sleep() {
	uint8_t i;
	const Sys_WakeupGPIO_t *gpio;

	SYS_UnlockReg();

	// In power down mode the only active clocks are LIRC and LXT.
	// All our pheriperials are clocked from HXT or PLL, so we
	// don't have to worry about stopping them.
	// Button debounce is clocked from LIRC so it will work.

	// Disable unwanted interrupts
	for(i = 0; i < 4; i++) {
		gpio = &Sys_wakeupGPIO[i];
		if(!(Sys_wakeupSource & gpio->wakeupMask)) {
			GPIO_DisableInt(gpio->port, gpio->pin);
		}
	}

	// Enable wakeup interrupt
	CLK->PWRCTL |= CLK_PWRCTL_PDWKIEN_Msk;
	NVIC_EnableIRQ(PWRWU_IRQn);
	// Sleep
	CLK_PowerDown();
	// Disable wakeup interrupt
	CLK->PWRCTL &= ~CLK_PWRCTL_PDWKIEN_Msk;
	NVIC_DisableIRQ(PWRWU_IRQn);

	// Re-enable previously disabled interrupts
	// All those interrupts are always enabled anyway,
	// so we just enable them all.
	for(i = 0; i < 4; i++) {
		gpio = &Sys_wakeupGPIO[i];
		GPIO_EnableInt(gpio->port, gpio->pin, GPIO_INT_BOTH_EDGE);
	}

	SYS_LockReg();
}

void Sys_SetWakeupSource(uint8_t source) {
	Sys_wakeupSource = source;
}

uint8_t Sys_GetLastWakeupSource() {
	return Sys_lastWakeupSource;
}
