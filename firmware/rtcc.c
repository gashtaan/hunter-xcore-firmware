/*
   https://github.com/gashtaan/hunter-xcore-firmware

   Copyright (C) 2021, Michal Kovacik

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rtcc.h"

datetime_t now;

// month days
static const uint8_t month_days_table[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
// first week-day for years 2000-27, then it repeats
static const uint8_t first_weekday_table[28] = { 5, 0, 1, 2, 3, 5, 6, 0, 1, 3, 4, 5, 6, 1, 2, 3, 4, 6, 0, 1, 2, 4, 5, 6, 0, 2, 3, 4 };

static const datetime_t default_datetime = { 0x21, 0x01, 0x01, 0x04, 0x12, 0x00, 0x00 };

static void rtcc_write_enable()
{
	EECON2 = 0x55;
	EECON2 = 0xAA;
	RTCCFGbits.RTCWREN = 1;
}
static void rtcc_write_disable()
{
	EECON2 = 0x55;
	EECON2 = 0xAA;
	RTCCFGbits.RTCWREN = 0;
}

bool rtcc_init(void)
{
	OSCCON2bits.SOSCGO = 1;

	if (!rtcc_enabled())
	{
		rtcc_enable();
		rtcc_set(&default_datetime, false);
		return false;
	}
	return true;
}

bool rtcc_enabled(void)
{
	return RTCCFGbits.RTCEN;
}
void rtcc_enable(void)
{
	rtcc_write_enable();
	RTCCFGbits.RTCEN = 1;
	rtcc_write_disable();
}

void rtcc_enable_alarm(void)
{
	// alarm on every minute at 0th second
	rtcc_write_enable();
	ALRMCFGbits.ALRMPTR = 0;
	ALRMVALL = 0;
	rtcc_write_disable();

	ALRMCFGbits.ALRMEN = 1;
	ALRMCFGbits.CHIME = 1;
	ALRMCFGbits.AMASK = 3;
}

void rtcc_fix(datetime_t* datetime)
{
	uint8_t month_days = rtcc_month_days(datetime);
	if (datetime->day > month_days)
		datetime->day = month_days;

	uint8_t year = bcd_to_number(datetime->year);
	while (year >= sizeof(first_weekday_table))
		year -= sizeof(first_weekday_table);

	uint8_t month = bcd_to_number(datetime->month) - 1;
	uint16_t day = bcd_to_number(datetime->day) - 1;
	for (uint8_t n = 0; n < month; ++n)
		day += month_days_table[n];
	if (month >= 1 && (year & 3) == 0)
		day++;
	day += first_weekday_table[year];

	datetime->weekday = day % 7;
}

void rtcc_set(const datetime_t* datetime, bool keep_seconds)
{
	rtcc_write_enable();

	RTCCFGbits.RTCPTR = 3;
	while (RTCCFGbits.RTCSYNC);
	RTCVALL = datetime->year;
	RTCVALH = 0;
	RTCVALL = datetime->day;
	RTCVALH = datetime->month;
	RTCVALL = datetime->hours;
	RTCVALH = datetime->weekday;
	if (!keep_seconds)
		RTCVALL = datetime->seconds;
	RTCVALH = datetime->minutes;

	rtcc_write_disable();
}

void rtcc_get(datetime_t* datetime)
{
	RTCCFGbits.RTCPTR = 3;
	while (RTCCFGbits.RTCSYNC);
	datetime->year = RTCVALL;
	uint8_t dummy = RTCVALH;
	datetime->day = RTCVALL;
	datetime->month = RTCVALH;
	datetime->hours = RTCVALL;
	datetime->weekday = RTCVALH;
	datetime->seconds = RTCVALL;
	datetime->minutes = RTCVALH;
}

void rtcc_sync()
{
	RTCCFGbits.RTCPTR = 3;
	while (RTCCFGbits.RTCSYNC);
	now.year = RTCVALL;
	uint8_t dummy = RTCVALH;
	now.day = RTCVALL;
	now.month = RTCVALH;
	now.hours = RTCVALL;
	now.weekday = RTCVALH;
	now.seconds = RTCVALL;
	now.minutes = RTCVALH;
}

uint8_t rtcc_month_days(datetime_t* datetime)
{
	uint8_t days = month_days_table[bcd_to_number(datetime->month) - 1];
	if (days == 28 && (datetime->year & 3) == 0)
		days++;
	return number_to_bcd(days);
}

uint16_t rtcc_year_day(const datetime_t* datetime)
{
	uint16_t day = bcd_to_number(datetime->day) - 1;
	uint8_t month = bcd_to_number(datetime->month) - 1;
	for (uint8_t n = 0; n < month; ++n)
		day += month_days_table[month];
	if (month >= 2 && (datetime->year & 3) == 0)
		day++;
	return day;
}

uint8_t bcd_to_number(uint8_t bcd)
{
	return ((bcd >> 4) * 10) + (bcd & 15);
}

uint8_t number_to_bcd(uint8_t number)
{
	uint8_t div10 = (number / 10);
	return (uint8_t)(div10 << 4) + (number - div10 * 10);
}