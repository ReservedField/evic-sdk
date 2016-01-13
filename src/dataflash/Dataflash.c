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
 * Copyright (C) 2016 Jussi Timperi
 */

#include <M451Series.h>
#include <Dataflash.h>

/* Offsets in the dataflash */
#define DATAFLASH_OFFSET_HWVER       0x04
#define DATAFLASH_OFFSET_STATUS      0x78
#define DATAFLASH_OFFSET_BOOTFLAG    0x09

Dataflash_Info_t Dataflash_info;

static uint32_t Dataflash_baseAddr;

/**
 * Finds the dataflash base address.
 * This is an internal function.
 *
 * @return The dataflash base address.
 */
static uint32_t Dataflash_GetBaseAddress() {
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

/**
 * Reads a single byte from the dataflash.
 * This is an internal function.
 *
 * @param addr Address to read from.
 *
 * @return Read byte.
 */
static uint8_t Dataflash_ReadByte(uint32_t addr) {
	uint32_t data;
	uint8_t shift;

	// Read the word containing the byte
	data = FMC_Read(addr & 0xFFFFFFFC);

	// Little-endian
	shift = (addr & 0x3) * 8;

	return (data >> shift) & 0xFF;
}

/**
 * Writes a byte to the dataflash.
 * This is an internal function.
 *
 * @param addr  Address to write to.
 * @param value Byte to write.
 */
static void Dataflash_WriteByte(uint32_t addr, uint8_t value) {
	uint32_t alignedAddr, data;
	uint8_t shift;

	// Read the word containing the byte
	alignedAddr = addr & 0xFFFFFFFC;
	data = FMC_Read(alignedAddr);

	// Little-endian
	shift = (addr & 0x3) * 8;

	// Write the updated word
	data &= ~(0xFF << shift);
	data |= value << shift;
	FMC_Write(alignedAddr, data);
}

/**
 * Writes the boot flag to boot from APROM.
 * This is an internal function.
 */
static void Dataflash_SetBootAPROM() {
	// Small hack:
	// An erased flash bit is 1, programmed is 0.
	// Since the APROM boot flag is zero, erasing is not required.
	// From datasheet: "minimum program bit size is 32 bits", so we're in the clear.
	Dataflash_WriteByte(Dataflash_baseAddr + DATAFLASH_OFFSET_BOOTFLAG, DATAFLASH_BOOTFLAG_APROM);
	Dataflash_info.bootFlag = Dataflash_ReadByte(Dataflash_baseAddr + DATAFLASH_OFFSET_BOOTFLAG);
}

void Dataflash_Init() {
	// Init FMC and find base
	FMC_Open();
	Dataflash_baseAddr = Dataflash_GetBaseAddress();

	// Populate info structure
	Dataflash_info.hwVersion = Dataflash_ReadByte(Dataflash_baseAddr + DATAFLASH_OFFSET_HWVER);
	Dataflash_info.status = FMC_Read(Dataflash_baseAddr + DATAFLASH_OFFSET_STATUS);
	Dataflash_info.bootFlag = Dataflash_ReadByte(Dataflash_baseAddr + DATAFLASH_OFFSET_BOOTFLAG);

	// Update boot flag
	if(Dataflash_info.bootFlag != DATAFLASH_BOOTFLAG_APROM) {
		Dataflash_SetBootAPROM();
	}

	// Close FMC
	FMC_Close();
}
