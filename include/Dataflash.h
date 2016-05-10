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

#ifndef EVICSDK_DATAFLASH_H
#define EVICSDK_DATAFLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum structure size.
 */
#define DATAFLASH_STRUCT_MAX_SIZE      (FMC_FLASH_PAGE_SIZE - 8)
/**
 * Maximum number of structures.
 */
#define DATAFLASH_STRUCT_MAX_COUNT     8
/**
 * Invalid structure magic value.
 */
#define DATAFLASH_STRUCT_INVALID_MAGIC 0x00FFFFFF

/**
 * Structure to hold information about a dataflash struct.
 */
typedef struct {
	/**< Magic value (24 bits). Upper 8 bits must be zero. */
	uint32_t magic;
	/**< Structure size. */
	uint16_t size;
} Dataflash_StructInfo_t;

/**
 * Initializes the dataflash library.
 * System control registers must be unlocked.
 */
void Dataflash_Init();

/**
 * Gets a list of magic numbers for structures present
 * int the dataflash.
 *
 * @param magicList An array at least DATAFLASH_STRUCT_MAX_COUNT
 *                  elements big to receive the magic numbers.
 *
 * @return Number of magic numbers stored in the array.
 */
uint8_t Dataflash_GetMagicList(uint32_t *magicList);

/**
 * Reads a structure from the dataflash.
 *
 * @param structInfo Structure info.
 * @param dst        Pointer to structure to be filled.
 *
 * @return True on success, false otherwise.
 */
uint8_t Dataflash_ReadStruct(const Dataflash_StructInfo_t *structInfo, void *dst);

/**
 * Selects the set of dataflash structures to be used from now on.
 * This can only be called once (further calls will be ignored) and must
 * have been called prior to performing updates (they'll fail otherwise).
 * NOTE: GCC and Clang went big-hammer on multiple-indirection const casts,
 * so I made structInfo not const just to avoid warnings. It will NOT
 * be modified.
 *
 * @param structInfo Array of pointers to structure infos.
 * @param count      Number of elements in the array.
 *
 * @return True on success, false if the arguments or the infos are not valid.
 */
uint8_t Dataflash_SelectStructSet(Dataflash_StructInfo_t **structInfo, uint8_t count);

/**
 * Updates/writes a structure in the dataflash.
 * The dataflash structure set must have already been selected, or
 * this will fail.
 * The update operation is a read-compare-write, i.e. it won't waste flash
 * space and cycles writing if the old structure is the same as the new one.
 * However, this function is not fast, so try minimizing calls to it.
 *
 * @param structInfo Structure info.
 * @param src        Pointer to structure to be stored.
 *
 * @return True on success, false otherwise.
 */
uint8_t Dataflash_UpdateStruct(const Dataflash_StructInfo_t *structInfo, void *src);

/**
 * Invalidates a structure in the dataflash.
 * This doesn't actually erase the structure, but marks it in a
 * way such that it won't be read by the dataflash routines.
 * NOTE: This is provided for development/debugging only. You should
 * never need this in production code. The right way to restore defaults
 * is to push an update with default values. If you absolutely need this,
 * and you are sure you can't do it any other way, then go ahead, it
 * should be safe. It will probably increase wear on some flash pages, though.
 *
 * @param structInfo Structure info.
 *
 * @return True on success, false if there is nothing to invalidate.
 */
uint8_t Dataflash_InvalidateStruct(const Dataflash_StructInfo_t *structInfo);

/**
 * Erases the whole dataflash.
 * This actually erases all the flash memory belonging to dataflash.
 * NOTE: This is provided for development/debugging only. Do NOT use this
 * in production code. The right way to restore defaults is to push an
 * update with default values. If that absolutely won't work for you, use
 * Dataflash_InvalidateStruct.
 *
 * @return True on success, false otherwise.
 */
uint8_t Dataflash_Erase();

#ifdef __cplusplus
}
#endif

#endif
