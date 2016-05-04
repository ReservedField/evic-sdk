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
#include <RTCUtils.h>

/**
 * True if RTC_Open has already been called.
 */
static uint8_t RTCUtils_isRTCOpen = 0;

void RTCUtils_SetDateTime(const RTCUtils_DateTime_t *dateTime) {
	S_RTC_TIME_DATA_T rtcData;

	// Populate RTC data
	rtcData.u32Year      = 2000 + dateTime->year;
	rtcData.u32Month     = dateTime->month;
	rtcData.u32Day       = dateTime->day;
	rtcData.u32DayOfWeek = dateTime->dayOfWeek;
	rtcData.u32Hour      = dateTime->hour;
	rtcData.u32Minute    = dateTime->minute;
	rtcData.u32Second    = dateTime->second;
	rtcData.u32TimeScale = RTC_CLOCK_24;

	if(!RTCUtils_isRTCOpen) {
		// Enable LXT clock
		SYS_UnlockReg();
		CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
		CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);
		CLK_EnableModuleClock(RTC_MODULE);
		SYS_LockReg();

		// Initialize RTC with supplied data
		RTC_Open(&rtcData);
		RTCUtils_isRTCOpen = 1;
	}
	else {
		// Update RTC data
		RTC_SetDateAndTime(&rtcData);
	}
}

void RTCUtils_GetDateTime(RTCUtils_DateTime_t *dateTime) {
	S_RTC_TIME_DATA_T rtcData;

	if(!RTCUtils_isRTCOpen) {
		return;
	}

	// Grab RTC data
	RTC_GetDateAndTime(&rtcData);

	// Populate dateTime
	dateTime->year      = rtcData.u32Year - 2000;
	dateTime->month     = rtcData.u32Month;
	dateTime->day       = rtcData.u32Day;
	dateTime->dayOfWeek = rtcData.u32DayOfWeek;
	dateTime->hour      = rtcData.u32Hour;
	dateTime->minute    = rtcData.u32Minute;
	dateTime->second    = rtcData.u32Second;
}
