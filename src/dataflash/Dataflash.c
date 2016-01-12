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
 * Copyright (C) 2016 Jussi Timperi
 */

#include <M451Series.h>
#include <Dataflash.h>

// Offsets in the dataflash
#define DATAFLASH_OFFSET_HWVER       0x04
#define DATAFLASH_OFFSET_STATUS      0x78

Dataflash_Info_t Dataflash_info;

/**
 * Finds the dataflash base address.
 * System control registers must be unlocked.
 *
 * @return The dataflash base address.
 */
static uint32_t Dataflash_getBaseAddress() {
	uint32_t base, offset;

	base = FMC_ReadDataFlashBaseAddr();
	for(offset = 0; offset < 0x1000; offset += 0x100) {
		if(FMC_Read(base + offset) == 0xFFFFFFFF && FMC_Read(base + offset + 4) == 0xFFFFFFFF) {
			break;
		}
	}

	if(offset != 0) {
		base += offset - 0x100;
	}

	return base;
}

void Dataflash_Init() {
	uint32_t base;

	// Init FMC and find base
	FMC_Open();
	base = Dataflash_getBaseAddress();

	// Populate info structure
	Dataflash_info.hwVersion = FMC_Read(base + DATAFLASH_OFFSET_HWVER) & 0xFF;
	Dataflash_info.status = FMC_Read(base + DATAFLASH_OFFSET_STATUS);

	// Close FMC
	FMC_Close();
}
