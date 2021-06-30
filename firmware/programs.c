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

#include "programs.h"
#include "eeprom.h"

program_t programs[NUMBER_OF_PROGRAMS];
uint8_t programs_seasonal_adjustment;

void programs_init(void)
{
	if (!eeprom_check())
	{
		programs_defaults();
		programs_save();
		eeprom_validate();
		return;
	}

	programs_restore();
}

void programs_defaults(void)
{
	program_t* program = &programs[0];
	for (uint8_t n = 0; n < NUMBER_OF_PROGRAMS; ++n, ++program)
	{
		program->start_time.hour = 0x24;
		program->start_time.minute = 0x00;
		program->calendar.weekdays_bit = 0;
		program->calendar.weekdays_mask = 0b1111111;

		for (uint8_t m = 0; m < NUMBER_OF_STATIONS; ++m)
			program->run_times[m] = 0;
	}

	// default seasonal adjustment is 100%
	programs_seasonal_adjustment = 10;
}

void programs_restore(void)
{
	eeprom_read_data((uint8_t*)&programs, sizeof(programs), 0x0010);
	eeprom_read_data((uint8_t*)&programs_seasonal_adjustment, sizeof(programs_seasonal_adjustment), 0x0010 + sizeof(programs));
}

void programs_save(void)
{
	eeprom_write_data((uint8_t*)&programs, sizeof(programs), 0x0010);
	eeprom_write_data((uint8_t*)&programs_seasonal_adjustment, sizeof(programs_seasonal_adjustment), 0x0010 + sizeof(programs));
}

void programs_check(const datetime_t* now)
{
	uint16_t year_day = rtcc_year_day(now);

	const program_t* program = &programs[0];
	for (uint8_t n = 0; n < NUMBER_OF_PROGRAMS; ++n, ++program)
	{
		program_start_time_t start_time = program->start_time;
		program_calendar_t calendar = program->calendar;

		if (!(start_time.hour == now->hours && start_time.minute == now->minutes))
			continue;

		if (calendar.weekdays_bit == 0)
		{
			if (!(calendar.weekdays_mask & (1 << now->weekday)))
				// program not enabled for current weekday
				continue;
		}
		else if (calendar.repeat_bits == 0b10)
		{
			if (calendar.offset != (year_day % (calendar.days + 1)))
				// program not enabled for current n-th day
				continue;
		}
		else if (calendar.odd_even_bits == 0b110)
		{
			if (calendar.odd_even == (year_day & 1))
				// program not enabled for current odd/even day
				continue;
		}

		programs_queue(program);
	}
}

bool programs_queue(const program_t* program)
{
	bool any_started = false;
	for (uint8_t m = 0; m < NUMBER_OF_STATIONS; ++m)
	{
		uint16_t run_time = program->run_times[m];
		if (run_time > 0)
		{
			run_time *= 6; // (60 seconds in minute / 10 base for adjustment)
			run_time *= programs_seasonal_adjustment;

			if (stations_queue_start(m, run_time))
				any_started = true;
		}
	}
	return any_started;
}

void programs_reset_calendar(const datetime_t* now)
{
	uint16_t year_day = rtcc_year_day(now);

	for (uint8_t n = 0; n < NUMBER_OF_PROGRAMS; ++n)
	{
		// reset calendar offset to this n-day
		if (programs[n].calendar.repeat_bits == 0b10)
			programs[n].calendar.offset = year_day % (programs[n].calendar.days + 1);
	}

	programs_save();
}