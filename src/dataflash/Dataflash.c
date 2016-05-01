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

#include <string.h>
#include <M451Series.h>
#include <Dataflash.h>

/* Number of pages in dataflash */
#define DATAFLASH_PAGE_COUNT DATAFLASH_STRUCT_MAX_COUNT

/* Mask for magic value */
#define DATAFLASH_MASK_MAGIC 0x00FFFFFF
/* Mask for MRP (most recent page) bit */
#define DATAFLASH_MASK_MRP   (1 << 31)
/* Mask for MRU (most recently used) bit */
#define DATAFLASH_MASK_MRU   (1 << 30)

/* Convenience macros to open/close FMC with sys regs unlock/lock */
#define DATAFLASH_FMC_OPEN()  do { SYS_UnlockReg(); FMC_Open(); FMC_ENABLE_ISP(); FMC_ENABLE_AP_UPDATE(); } while(0)
#define DATAFLASH_FMC_CLOSE() do { FMC_DISABLE_AP_UPDATE(); FMC_DISABLE_ISP(); FMC_Close(); SYS_LockReg(); } while(0)

/* Macro to calculate our dataflash base address */
#define DATAFLASH_READ_BASEADDR() (FMC_ReadDataFlashBaseAddr() - DATAFLASH_PAGE_COUNT * FMC_FLASH_PAGE_SIZE)

/* Page flag is not initialized yet */
#define DATAFLASH_PAGEFLAG_INIT  0x00
/* Page is not yet marked good, but block info has been read */
#define DATAFLASH_PAGEFLAG_INFO  0x01
/* Page contains recent data */
#define DATAFLASH_PAGEFLAG_GOOD  0x02
/* Page can be erased */
#define DATAFLASH_PAGEFLAG_STALE 0x03
/* Page is erased */
#define DATAFLASH_PAGEFLAG_CLEAN 0x04

/**
 * Structure to hold block information.
 */
typedef struct {
	/**< Block start address (count word address). */
	uint32_t addr;
	/**< Decoded structure count. */
	uint8_t count;
} Dataflash_BlockInfo_t;

/**
 * Structure to hold page information.
 */
typedef struct {
	/**< Magic word. */
	uint32_t magicWord;
	/**< One of DATAFLASH_PAGEFLAG_*. */
	uint8_t flag;
	/**< Most recent block info. Only valid if flag is DATAFLASH_PAGEFLAG_INFO/GOOD. */
	Dataflash_BlockInfo_t blockInfo;
} Dataflash_PageInfo_t;

/**
 * Array of info structures for each page.
 */
static Dataflash_PageInfo_t Dataflash_pageInfo[DATAFLASH_PAGE_COUNT];

/**
 * Index of the most recently used good page.
 */
static uint8_t Dataflash_mruPage;

/**
 * True if a structure set has been selected.
 */
static uint8_t Dataflash_structSetSelected;

/**
 * Decodes a unary-coded word. The decoded value is equal
 * to the number of trailing (on the right) zero bits + 1.
 * This is an internal function.
 *
 * @param word Word to decode.
 *
 * @return Decoded value.
 */
static uint8_t Dataflash_UnaryDecode(uint32_t word) {
	uint8_t val;

	val = 1;
	while(!(word & 1)) {
		word >>= 1;
		val++;
	}

	return val;
}

/**
 * Encodes a unary-coded word. The rightmost val - 1 bits will
 * be zero, while the others will be one.
 * This is an internal function.
 *
 * @param val Value to encode. Valid range: 1 - 33.
 *
 * @return Encoded word.
 */
static uint32_t Dataflash_UnaryEncode(uint8_t val) {
	// For val == 33, 1 << (val - 1) overflows
	return val == 33 ? 0x00000000 : ~((1 << (val - 1)) - 1);
}

/**
 * Finds the most recent page for a structure.
 * This is an internal function.
 *
 * @param structInfo Structure info.
 * @param onlyGood   True to only search GOOD pages, false to also include INIT and INFO.
 *
 * @return Page index or a negative number if not found.
 */
static int8_t Dataflash_GetStructPageIndex(const Dataflash_StructInfo_t *structInfo, uint8_t onlyGood) {
	uint8_t i, flag;

	// Find most recent page for struct
	// Old pages are already marked STALE, no need to check MRP
	for(i = 0; i < DATAFLASH_PAGE_COUNT; i++) {
		flag = Dataflash_pageInfo[i].flag;
		if(((onlyGood && flag == DATAFLASH_PAGEFLAG_GOOD) ||
		   (!onlyGood && flag != DATAFLASH_PAGEFLAG_STALE && flag != DATAFLASH_PAGEFLAG_CLEAN)) &&
		   (Dataflash_pageInfo[i].magicWord & DATAFLASH_MASK_MAGIC) == structInfo->magic) {
			return i;
		}
	}

	// Page not found
	return -1;
}

/**
 * Finds the most recent block for the specified page.
 * This is an internal function.
 *
 * @param page       Page index. Valid range: 0 - (DATAFLASH_PAGE_COUNT-1).
 * @param structInfo Structure info.
 * @param blockInfo  Pointer to receive block info.
 */
static void Dataflash_ReadPageBlockInfo(uint8_t page, const Dataflash_StructInfo_t *structInfo, Dataflash_BlockInfo_t *blockInfo) {
	uint32_t addr, blockAddr, endAddr, countWord;

	addr = blockAddr = DATAFLASH_READ_BASEADDR() + page * FMC_FLASH_PAGE_SIZE + 4;
	endAddr = addr + FMC_FLASH_PAGE_SIZE - 4;
	while(addr < endAddr) {
		blockAddr = addr;

		countWord = FMC_Read(addr);
		if(countWord != 0x00000000) {
			// Count < 33, found most recent block
			break;
		}

		// Count == 33, advance (up to endAddr)
		addr += 4 + 33 * ((structInfo->size + 3) & ~3);
	}

	// blockAddr points to the most recent block
	blockInfo->addr = blockAddr;
	// countWord holds the counter for that block
	blockInfo->count = Dataflash_UnaryDecode(countWord);
}

/**
 * Checks if a page is fully erased.
 * This is an internal function.
 *
 * @param pageAddr Page start address.
 * @param offset   Number of bytes to skip from page start.
 *
 * @return True if the page is erased, false otherwise.
 */
static uint8_t Dataflash_IsPageClean(uint32_t pageAddr, uint16_t offset) {
	for(; offset < FMC_FLASH_PAGE_SIZE && FMC_Read(pageAddr + offset) == 0xFFFFFFFF;
	    offset += 4);

	return offset == FMC_FLASH_PAGE_SIZE;
}

/**
 * Prepares a new page for a struct from the CLEAN/STALE pool,
 * initializes a new block and sets its struct count to 1.
 * Optionally, clears the MRP bit for the old page.
 * This is an internal function.
 *
 * @param structInfo Structure info.
 * @param oldPage    Old page index, or a negative number.
 * 
 * @return Index of the new page on success, a negative number otherwise.
 */
static uint8_t Dataflash_AllocatePage(const Dataflash_StructInfo_t *structInfo, int8_t oldPage) {
	uint8_t i, pageIndex, flag;
	int8_t cleanIndex = -1, staleIndex = -1;
	uint32_t baseAddr, pageAddr, magicWord;

	// Find a CLEAN/STALE page, preferring CLEAN
	// Start searching from the page following the MRU page
	for(i = 0; i < DATAFLASH_PAGE_COUNT && cleanIndex == -1; i++) {
		pageIndex = (Dataflash_mruPage + 1 + i) % DATAFLASH_PAGE_COUNT;
		flag = Dataflash_pageInfo[pageIndex].flag;

		if(cleanIndex == -1 && flag == DATAFLASH_PAGEFLAG_CLEAN) {
			cleanIndex = pageIndex;
		}
		else if(staleIndex == -1 && flag == DATAFLASH_PAGEFLAG_STALE) {
			staleIndex = pageIndex;
		}
	}

	if(cleanIndex == -1 && staleIndex == -1) {
		// No pages available
		return -1;
	}

	baseAddr = DATAFLASH_READ_BASEADDR();

	if(cleanIndex == -1) {
		// No clean pages available
		// Erase stale page
		if(FMC_Erase(baseAddr + staleIndex * FMC_FLASH_PAGE_SIZE) != 0) {
			// Erase failed
			return -1;
		}
		cleanIndex = staleIndex;

		// Clear MRU bit from ex-MRU page and update Dataflash_mruPage
		magicWord = Dataflash_pageInfo[Dataflash_mruPage].magicWord & ~DATAFLASH_MASK_MRU;
		FMC_Write(baseAddr + Dataflash_mruPage * FMC_FLASH_PAGE_SIZE, magicWord);
		Dataflash_pageInfo[Dataflash_mruPage].magicWord = magicWord;
		Dataflash_mruPage = cleanIndex;
	}

	// Write magic word, MRP set, MRU set
	// Set reserved bits to zero
	pageAddr = baseAddr + cleanIndex * FMC_FLASH_PAGE_SIZE;
	magicWord = (structInfo->magic & DATAFLASH_MASK_MAGIC) | DATAFLASH_MASK_MRP | DATAFLASH_MASK_MRU;
	FMC_Write(pageAddr, magicWord);

	// Struct counter is erased, i.e. 1
	Dataflash_pageInfo[cleanIndex].magicWord = magicWord;
	Dataflash_pageInfo[cleanIndex].flag = DATAFLASH_PAGEFLAG_GOOD;
	Dataflash_pageInfo[cleanIndex].blockInfo.addr = pageAddr + 4;
	Dataflash_pageInfo[cleanIndex].blockInfo.count = 1;

	if(oldPage >= 0 && oldPage < DATAFLASH_PAGE_COUNT) {
		// Clear MRP from old page
		magicWord = Dataflash_pageInfo[oldPage].magicWord & ~DATAFLASH_MASK_MRP;
		FMC_Write(baseAddr + oldPage * FMC_FLASH_PAGE_SIZE, magicWord);
		Dataflash_pageInfo[oldPage].magicWord = magicWord;
		Dataflash_pageInfo[oldPage].flag = DATAFLASH_PAGEFLAG_STALE;
	}

	return cleanIndex;
}

/**
 * Reads data from the dataflash.
 * This is an internal function.
 *
 * @param dst  Destination buffer.
 * @param src  Dataflash source address, must be word-aligned.
 * @param size Copy size.
 */
static void Dataflash_Read(uint8_t *dst, uint32_t src, uint32_t size) {
	uint32_t word;

	// Copy full words
	while(size >= 4) {
		*((uint32_t *) dst) = FMC_Read(src);
		dst += 4;
		src += 4;
		size -= 4;
	}

	if(size) {
		// Copy remaining bytes
		word = FMC_Read(src);
		memcpy(dst, &word, size);
	}
}

/**
 * Writes data to the dataflash.
 * This is an internal function.
 *
 * @param dst  Dataflash destination address, must be work-aligned.
 * @param src  Source buffer.
 * @param size Copy size.
 */
static void Dataflash_Write(uint32_t dst, const uint8_t *src, uint32_t size) {
	uint32_t word;

	// Multi-word programming is not used because it shouldn't
	// execute from flash to avoid CPU fetch hold.

	// Copy full words
	while(size >= 4) {
		FMC_Write(dst, *((const uint32_t *) src));
		dst += 4;
		src += 4;
		size -= 4;
	}

	if(size) {
		// Copy remaining bytes
		word = 0xFFFFFFFF;
		memcpy(&word, src, size);
		FMC_Write(dst, word);
	}
}

void Dataflash_Init() {
	uint8_t i;
	uint32_t baseAddr, pageAddr, word;

	Dataflash_mruPage = DATAFLASH_PAGE_COUNT - 1;
	Dataflash_structSetSelected = 0;

	FMC_Open();

	baseAddr = DATAFLASH_READ_BASEADDR();
	for(i = 0; i < DATAFLASH_PAGE_COUNT; i++) {
		// Read magic word
		pageAddr = baseAddr + i * FMC_FLASH_PAGE_SIZE;
		word = FMC_Read(pageAddr);

		// If the word is all 1s, the page might already be erased
		if(word == 0xFFFFFFFF && Dataflash_IsPageClean(pageAddr, 4)) {
			// Page is erased, mark CLEAN
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_CLEAN;
		}
		else if((word & DATAFLASH_MASK_MAGIC) == DATAFLASH_STRUCT_INVALID_MAGIC ||
		        !(word & DATAFLASH_MASK_MRP)) {
			// Magic is invalid or MRP is not set
			// Page can be erased, mark STALE
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_STALE;
		}
		else {
			// Page is potentially GOOD, mark as INIT
			// Set reserved bits to zero
			Dataflash_pageInfo[i].magicWord = word & (DATAFLASH_MASK_MAGIC | DATAFLASH_MASK_MRP | DATAFLASH_MASK_MRU);
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_INIT;
		}

		if(Dataflash_pageInfo[i].flag != DATAFLASH_PAGEFLAG_CLEAN && (word & DATAFLASH_MASK_MRU)) {
			// The MRU page is not necessarily a GOOD page.
			// For example, the most recent GOOD page may have been created
			// on an erased page, for which we don't set MRU. If the flash
			// is well-formed, there should be only one non-erased page with
			// the MRU bit set. If it isn't, we can't really do much about it,
			// so we assume the last page with MRU set is it.
			Dataflash_mruPage = i;
		}
	}

	FMC_Close();
}

uint8_t Dataflash_GetMagicList(uint32_t *magicList) {
	uint8_t i, j, magicCount, flag;
	uint32_t magic;

	magicCount = 0;
	for(i = 0; i < DATAFLASH_PAGE_COUNT; i++) {
		flag = Dataflash_pageInfo[i].flag;
		if(flag == DATAFLASH_PAGEFLAG_STALE || flag == DATAFLASH_PAGEFLAG_CLEAN) {
			// Skip stale/clean pages
			continue;
		}

		// Isolate magic
		magic = Dataflash_pageInfo[i].magicWord & DATAFLASH_MASK_MAGIC;

		// Check if this magic is already in the list
		for(j = 0; j < magicCount && magicList[j] != magic; j++);
		if(j == magicCount) {
			// Not in list, add it
			magicList[j] = magic;
			magicCount++;
		}
	}

	return magicCount;
}

uint8_t Dataflash_ReadStruct(const Dataflash_StructInfo_t *structInfo, void *dst) {
	Dataflash_PageInfo_t *pageInfo;
	int8_t pageIndex;
	uint32_t addr;

	if((structInfo->magic & DATAFLASH_MASK_MAGIC) == DATAFLASH_STRUCT_INVALID_MAGIC ||
	   structInfo->size > DATAFLASH_STRUCT_MAX_SIZE) {
		// Invalid magic or size
		return 0;
	}

	// Find most recent page for struct
	pageIndex = Dataflash_GetStructPageIndex(structInfo, 0);
	if(pageIndex < 0) {
		return 0;
	}
	pageInfo = &Dataflash_pageInfo[pageIndex];

	DATAFLASH_FMC_OPEN();

	if(pageInfo->flag == DATAFLASH_PAGEFLAG_INIT) {
		// Fill in most recent block info
		Dataflash_ReadPageBlockInfo(pageIndex, structInfo, &pageInfo->blockInfo);
		pageInfo->flag = DATAFLASH_PAGEFLAG_INFO;
	}

	// Most recent update is at index (pageInfo->blockInfo.count - 1)
	addr = pageInfo->blockInfo.addr + 4 + (pageInfo->blockInfo.count - 1) * ((structInfo->size + 3) & ~3);
	Dataflash_Read(dst, addr, structInfo->size);

	DATAFLASH_FMC_CLOSE();

	return 1;
}

uint8_t Dataflash_SelectStructSet(Dataflash_StructInfo_t **structInfo, uint8_t count) {
	uint8_t i, j, flag;
	uint32_t magic;

	if(Dataflash_structSetSelected || count > DATAFLASH_STRUCT_MAX_COUNT) {
		// Already executed or invalid count
		return 0;
	}

	// Sanity check on structures
	for(i = 0; i < count &&
	    (structInfo[i]->magic & DATAFLASH_MASK_MAGIC) != DATAFLASH_STRUCT_INVALID_MAGIC &&
	    structInfo[i]->size <= DATAFLASH_STRUCT_MAX_SIZE; i++);
	if(i != count) {
		// Invalid magic or size
		return 0;
	}

	DATAFLASH_FMC_OPEN();

	for(i = 0; i < DATAFLASH_PAGE_COUNT; i++) {
		// Skip (GOOD)/STALE/CLEAN
		// Since this executes only once there shouldn't be GOOD pages
		flag = Dataflash_pageInfo[i].flag;
		if(flag != DATAFLASH_PAGEFLAG_INIT && flag != DATAFLASH_PAGEFLAG_INFO) {
			continue;
		}

		// Find structure for page magic
		magic = Dataflash_pageInfo[i].magicWord & DATAFLASH_MASK_MAGIC;
		for(j = 0; j < count && structInfo[j]->magic != magic; j++);
		if(j == count) {
			// Structure not in set, mark as STALE
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_STALE;
		}
		else {
			if(flag == DATAFLASH_PAGEFLAG_INIT) {
				// Fill in most recent block info
				Dataflash_ReadPageBlockInfo(i, structInfo[j], &Dataflash_pageInfo[i].blockInfo);
			}

			// Magic found and page is the most recent for this struct
			// If it wasn't it would've been already marked STALE
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_GOOD;
		}
	}

	DATAFLASH_FMC_CLOSE();
	Dataflash_structSetSelected = 1;

	return 1;
}

uint8_t Dataflash_UpdateStruct(const Dataflash_StructInfo_t *structInfo, void *src) {
	int8_t oldPage, newPage;
	uint8_t allocPage;
	uint16_t structSizeAlign;
	uint32_t baseAddr, endAddr, dstAddr;
	Dataflash_BlockInfo_t *blockInfo;

	if(!Dataflash_structSetSelected) {
		// Dataflash_SelectStructSet() hasn't been called yet
		return 0;
	}

	DATAFLASH_FMC_OPEN();
	baseAddr = DATAFLASH_READ_BASEADDR();
	structSizeAlign = (structInfo->size + 3) & ~3;
	allocPage = 1;

	// Find most recent GOOD page for this struct
	oldPage = Dataflash_GetStructPageIndex(structInfo, 1);
	if(oldPage >= 0) {
		// Page found, check whether an update can fit in it
		blockInfo = &Dataflash_pageInfo[oldPage].blockInfo;
		// If blockInfo->count == 33 we'll need to create a new block (extra 4 bytes)
		dstAddr = blockInfo->addr + blockInfo->count * structSizeAlign + (blockInfo->count == 33 ? 8 : 4);
		endAddr = baseAddr + (oldPage + 1) * FMC_FLASH_PAGE_SIZE;
		if(dstAddr + structSizeAlign <= endAddr) {
			// Fits in this page
			allocPage = 0;

			if(blockInfo->count == 33) {
				// Will create a new block
				// Counter word is already erased (i.e. 1)
				// dstAddr will skip over the counter word
				blockInfo->addr = dstAddr - 4;
				blockInfo->count = 1;
			}
			else {
				// Increment struct counter
				blockInfo->count++;
				FMC_Write(blockInfo->addr, Dataflash_UnaryEncode(blockInfo->count));
			}
		}
	}

	if(allocPage) {
		// Allocate page and setup first block
		newPage = Dataflash_AllocatePage(structInfo, oldPage);
		if(newPage < 0) {
			// Allocation failed
			DATAFLASH_FMC_CLOSE();
			return 0;
		}
		dstAddr = Dataflash_pageInfo[newPage].blockInfo.addr + 4;
	}

	// Write update to flash
	Dataflash_Write(dstAddr, src, structInfo->size);

	DATAFLASH_FMC_CLOSE();

	return 1;
}

uint8_t Dataflash_InvalidateStruct(const Dataflash_StructInfo_t *structInfo) {
	uint8_t ret, i;
	uint32_t baseAddr, magicWord, magicMask;

	ret = 0;
	DATAFLASH_FMC_OPEN();
	baseAddr = DATAFLASH_READ_BASEADDR();

	// Operate directly on pageInfo to cover all corner cases
	// Look up MRP pages for this magic
	magicMask = (structInfo->magic & DATAFLASH_MASK_MAGIC) | DATAFLASH_MASK_MRP;
	for(i = 0; i < DATAFLASH_PAGE_COUNT; i++) {
		magicWord = Dataflash_pageInfo[i].magicWord;
		if((magicWord & magicMask) == magicMask) {
			// Unset MRP bit
			magicWord &= ~DATAFLASH_MASK_MRP;
			FMC_Write(baseAddr + i * FMC_FLASH_PAGE_SIZE, magicWord);

			// Update page info
			Dataflash_pageInfo[i].magicWord = magicWord;
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_STALE;

			ret = 1;
		}
	}

	DATAFLASH_FMC_CLOSE();

	return ret;
}

uint8_t Dataflash_Erase() {
	uint8_t ret, i;
	uint32_t baseAddr;

	ret = 1;
	DATAFLASH_FMC_OPEN();
	baseAddr = DATAFLASH_READ_BASEADDR();

	for(i = 0; i < DATAFLASH_PAGE_COUNT; i++) {
		if(FMC_Erase(baseAddr + i * FMC_FLASH_PAGE_SIZE) == 0) {
			// Erase succeeded, mark as clean
			Dataflash_pageInfo[i].magicWord = 0xFFFFFFFF;
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_CLEAN;
		}
		else {
			// Erase failed, continue anyway
			// Mark as stale for this run
			Dataflash_pageInfo[i].flag = DATAFLASH_PAGEFLAG_STALE;
			ret = 0;
		}
	}

	DATAFLASH_FMC_CLOSE();

	return ret;
}
