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

#pragma once

#include "types.h"

typedef struct
{
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t weekday;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} datetime_t;

extern datetime_t now;

bool rtcc_init(void);
bool rtcc_enabled(void);
void rtcc_enable(void);
void rtcc_enable_alarm(void);
void rtcc_fix(datetime_t* datetime);
void rtcc_set(const datetime_t* datetime, bool keep_seconds);
void rtcc_get(datetime_t* datetime);
void rtcc_sync(void);
uint8_t rtcc_month_days(datetime_t* datetime);
uint16_t rtcc_year_day(const datetime_t* datetime);

uint8_t bcd_to_number(uint8_t bcd);
uint8_t number_to_bcd(uint8_t number);
