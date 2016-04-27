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
#include <SysInfo.h>

/* OFW config structure size */
#define SYSINFO_STRUCT_SIZE     0x100
/* Pages for OFW config struct wear leveling */
#define SYSINFO_STRUCT_PAGES    2
/* Offsets in the OFW config struct */
#define SYSINFO_OFFSET_HWVER    0x04
#define SYSINFO_OFFSET_STATUS   0x78
#define SYSINFO_OFFSET_BOOTFLAG 0x09
/* Status flag for flipped display */
#define SYSINFO_STATUS_FLIP     0x04

SysInfo_Info_t gSysInfo;

/**
 * Finds the address of the most recent OFW config update.
 * This is an internal function.
 * 
 * @param configAddr Pointer to receive the address.
 *
 * @return True on success, false otherwise.
 */
static uint8_t SysInfo_GetConfigAddress(uint32_t *configAddr) {
	uint32_t baseAddr, endAddr, addr;

	// Find first occurrence of 2 erased words at struct start
	baseAddr = FMC_ReadDataFlashBaseAddr();
	endAddr = baseAddr + FMC_FLASH_PAGE_SIZE * SYSINFO_STRUCT_PAGES;
	for(addr = baseAddr; addr < endAddr; addr += SYSINFO_STRUCT_SIZE) {
		if(FMC_Read(addr) == 0xFFFFFFFF && FMC_Read(addr + 4) == 0xFFFFFFFF) {
			break;
		}
	}

	if(addr == baseAddr) {
		// No data found
		return 0;
	}

	// addr points to erased data or endAddr
	// Most recent struct is the previous one
	*configAddr = addr - SYSINFO_STRUCT_SIZE;
	return 1;
}

/**
 * Reads a byte from the dataflash.
 * This is an internal function.
 *
 * @param addr Address to read from.
 *
 * @return Read byte.
 */
static uint8_t SysInfo_ReadByte(uint32_t addr) {
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
static void SysInfo_WriteByte(uint32_t addr, uint8_t value) {
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

void SysInfo_Init() {
	uint32_t configAddr;

	FMC_Open();

	// Get address of most recent config update
	if(!SysInfo_GetConfigAddress(&configAddr)) {
		// This state should allow the firmware to run anyway
		// Hardware version is zero to signal something's wrong
		gSysInfo.hwVersion = 0;
		gSysInfo.displayFlip = 0,
		gSysInfo.bootFlag = SYSINFO_BOOTFLAG_APROM;

		FMC_Close();
		return;
	}

	// Populate system info structure
	gSysInfo.hwVersion = SysInfo_ReadByte(configAddr + SYSINFO_OFFSET_HWVER);
	gSysInfo.displayFlip = (FMC_Read(configAddr + SYSINFO_OFFSET_STATUS) & SYSINFO_STATUS_FLIP) ? 1 : 0;
	gSysInfo.bootFlag = SysInfo_ReadByte(configAddr + SYSINFO_OFFSET_BOOTFLAG);

	// Update boot flag
	if(gSysInfo.bootFlag != SYSINFO_BOOTFLAG_APROM) {
		// Small hack:
		// Programming flash bits (1 -> 0) doesn't require erasing
		// APROM bootflag is zero, so we can just write it
		SysInfo_WriteByte(configAddr + SYSINFO_OFFSET_BOOTFLAG, SYSINFO_BOOTFLAG_APROM);
		gSysInfo.bootFlag = SysInfo_ReadByte(configAddr + SYSINFO_OFFSET_BOOTFLAG);
	}

	FMC_Close();
}
