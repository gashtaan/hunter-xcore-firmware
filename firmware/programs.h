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
#include "rtcc.h"
#include "stations.h"

#define NUMBER_OF_PROGRAMS 8

typedef struct
{
	uint8_t hour; // 0x24 means OFF
	uint8_t minute;
} program_start_time_t;

typedef union
{
	// encoded information
	// 0XXXXXXX - weekday mask
	// 10XXXYYY - repeat days/offset
	// 110X0000 - odd/even
	struct
	{
		uint8_t weekdays_mask:7;
		uint8_t weekdays_bit:1;
	};
	struct
	{
		uint8_t offset:3;
		uint8_t days:3;
		uint8_t repeat_bits:2;
	};
	struct
	{
		uint8_t unused:4;
		uint8_t odd_even:1;
		uint8_t odd_even_bits:3;
	};
} program_calendar_t;

typedef struct
{
	program_start_time_t start_time;
	program_calendar_t calendar;
	uint8_t run_times[NUMBER_OF_STATIONS];

} program_t;

extern program_t programs[NUMBER_OF_PROGRAMS];
extern uint8_t programs_seasonal_adjustment;

void programs_init(void);
void programs_defaults(void);
void programs_restore(void);
void programs_save(void);

void programs_check(const datetime_t* now);
bool programs_queue(const program_t* program);
void programs_reset_calendar(const datetime_t* now);
