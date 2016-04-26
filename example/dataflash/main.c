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
#include <Dataflash.h>
#include <Button.h>
#include <TimerUtils.h>

// 24-bit structure magic number. Upper 8 bits must be zero.
// Each structure has its own magic number.
// If anything is changed within a structure, you MUST
// change the magic number.
// Contact me (ReservedField) to get a nice chunk of magic
// numbers assigned if you plan to publicy release software.
// A random number doesn't ensure nobody else is using it!
#define MYSTRUCT_MAGIC 0x000001

// Define one or more structures for your data.
// You should group data together by update
// frequency: a typical example is a big structure for
// rarely updated data and a small one for often updated
// data. This maximizes wear leveling efficiency.
// You can define up to DATAFLASH_STRUCT_MAX_COUNT structs.
// The maximum size for each is DATAFLASH_STRUCT_MAX_SIZE.
typedef struct {
	uint32_t count;
} MyDataflashStruct_t;

int main() {
	char buffer[100];
	MyDataflashStruct_t myStruct;
	Dataflash_StructInfo_t structInfo;
	Dataflash_StructInfo_t *structList[1];
	uint32_t magicList[DATAFLASH_STRUCT_MAX_COUNT];
	uint8_t magicCount, i, btnState, update;

	// Populate structure info
	structInfo.magic = MYSTRUCT_MAGIC;
	structInfo.size = sizeof(MyDataflashStruct_t);

	// First, we read the list of magics in the dataflash.
	// Other than checking whether your data is already on
	// flash, you can use this when updating you struct format:
	// scan for older versions and update them.
	magicCount = Dataflash_GetMagicList(magicList);

	// Let's see if our structure is there.
	// If you have multiple structs, one of them being
	// there does NOT assure the others are there too!
	// For example, the user may have cycled through different
	// firmwares.
	for(i = 0; i < magicCount && magicList[i] != MYSTRUCT_MAGIC; i++);
	if(i < magicCount) {
		// Found our struct, read it out
		if(!Dataflash_ReadStruct(&structInfo, &myStruct)) {
			// Uh oh, the magic's there but the read failed.
			// This should never happen if the arguments are
			// valid. In this example we halt execution.
			// You may want to be more graceful.
			Display_PutText(0, 0, "ERROR 1", FONT_DEJAVU_8PT);
			Display_Update();
			while(1);
		}
	}

	// Populate and select structure set.
	// This is the list of structures you're going
	// to use for the rest of execution. You can't change
	// it once it's set. If you want to upgrade from older
	// formats, read them out before selecting, convert them
	// to the new format and Update() them after selecting the
	// new set of structures.
	// You can't Update() until you call this.
	structList[0] = &structInfo;
	if(!Dataflash_SelectStructSet(structList, 1)) {
		// Again, this should never happen if the
		// arguments are valid.
		Display_PutText(0, 0, "ERROR 2", FONT_DEJAVU_8PT);
		Display_Update();
		while(1);
	}

	if(i == magicCount) {
		// We didn't find a struct to read out earlier.
		// Initialize a new struct and store it.
		myStruct.count = 0;
		// I won't error-check updates in this example
		Dataflash_UpdateStruct(&structInfo, &myStruct);
	}

	while(1) {
		// Handle button increment/decrement
		// A 500ms delay slows down updating
		update = 0;
		btnState = Button_GetState();
		if(btnState & BUTTON_MASK_RIGHT) {
			myStruct.count++;
			update = 1;
			Timer_DelayMs(500);
		}
		else if(btnState & BUTTON_MASK_LEFT) {
			if(myStruct.count != 0) {
				myStruct.count--;
				update = 1;
			}
			Timer_DelayMs(500);
		}

		if(update) {
			// Store new data
			Dataflash_UpdateStruct(&structInfo, &myStruct);
		}

		// Display counter
		siprintf(buffer, "Count:\n%lu", myStruct.count);
		Display_Clear();
		Display_PutText(0, 0, buffer, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
