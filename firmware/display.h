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

enum calendar_icons_t
{
	ICON_TIME_COMMA = 1,
	ICON_TIME_24HR = 2,
	ICON_TIME_AM = 4,
	ICON_TIME_PM = 8,
	ICON_DATE_MONTH = 16,
	ICON_DATE_DAY = 32,
	ICON_DATE_ODD = 64,
	ICON_DATE_EVEN = 128
};

enum other_icons_t
{
	ICON_CYCLE = 1,
	ICON_SOAK = 2,
	ICON_UMBRELLA = 4,
	ICON_SPRAY = 8,
	ICON_SPRAY_STOP = 16,
	ICON_SAND_CLOCK = 32,
	ICON_ALARM_CLOCK = 64,
	ICON_PERCENT = 128
};

void display_init(void);
void display_update(void);

void display_all(void);
void display_clear(void);

void display_digit(uint8_t position, uint8_t bits);
void display_digit_segments(uint8_t position, uint8_t bits);
void display_bars(uint16_t bits);
void display_weekdays(uint8_t bits);
void display_drops(uint8_t bits, uint8_t bits_stops);

void display_set_calendar_icons(uint8_t mask);
void display_set_other_icons(uint8_t mask);