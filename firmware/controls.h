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

enum selection_t
{
	// rotary controller functions
	FUNCTION_RUN = 0,
	FUNCTION_CURRENT_TIME = 1,
	FUNCTION_START_TIMES = 2,
	FUNCTION_RUN_TIMES = 3,
	FUNCTION_CALENDAR = 4,
	FUNCTION_SEASONAL_ADJUSTMENT = 5,
	FUNCTION_SOLAR_SYNC = 6,
	FUNCTION_MANUAL = 7,
	FUNCTION_OFF = 8,

	// virtual functions (initiated by long button-press)
	FUNCTION_START_STATIONS = 32,
	FUNCTION_PROGRESS = 33,

	FUNCTION_NONE = 255
};

enum buttons_t
{
	MINUS = 1,
	PLUS = 2,
	PRG = 4,
	RIGHT = 8,
	LEFT = 16
};

void controls_init(void);
uint8_t controls_selection(void);
uint8_t controls_buttons(void);
