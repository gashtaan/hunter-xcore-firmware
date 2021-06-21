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

#include "ui.h"
#include "display.h"
#include "rtcc.h"
#include "controls.h"
#include "programs.h"
#include "sensor.h"

uint8_t selection = 0;
uint8_t selection_previous = 0;

uint8_t selection_tab = 0;

uint8_t program_number = 0;
bool programs_changed = false;

uint8_t blink_delay = 0;
static bool in_blink(void);

uint8_t buttons = 0;
uint8_t buttons_repeat = 0;
static void update_buttons(void);

uint8_t idle_time = 0;

extern volatile bool ac_sensed;
extern volatile bool rain_sensed;
extern volatile bool overcurrent_detected;

extern volatile uint8_t ticks_fractions;

static uint8_t update_number(uint8_t number, uint8_t min, uint8_t max, bool increase);
static uint8_t update_bcd(uint8_t number, uint8_t min, uint8_t max, bool increase);

void ui_init(void)
{
	display_init();
	controls_init();
}

void ui_update(void)
{
	display_clear();

	update_buttons();

	if (overcurrent_detected && ac_sensed)
	{
		// display ERR until any button is pressed
		if (!buttons)
		{
			if (!(ticks_fractions & 4))
			{
				display_digit_segments(3, 0b01011011);
				display_digit_segments(4, 0b00011000);
				display_digit_segments(5, 0b00011000);
				display_update();
			}
			return;
		}

		overcurrent_detected = false;
	}

	// button right pressed longer that 3 second selects FUNCTION_START_STATIONS
	if (buttons == RIGHT && buttons_repeat == 24)
		ui_change_selection(FUNCTION_START_STATIONS);

	// increment idle time, or reset it when any button is pressed or rotary controller changed
	if (idle_time < 255)
		++idle_time;
	if (buttons)
		idle_time = 0;

	// update selection if rotary controller changed
	uint8_t s = controls_selection();
	if (s != FUNCTION_NONE && s != selection_previous)
	{
		ui_change_selection(s);
		idle_time = 0;
	}

	if (programs_changed && !buttons)
	{
		programs_changed = false;
		programs_save();
	}

	switch (selection)
	{
		case FUNCTION_RUN:
			ui_run();
			break;
		case FUNCTION_CURRENT_TIME:
			ui_current_time();
			break;
		case FUNCTION_START_TIMES:
			ui_start_times();
			break;
		case FUNCTION_RUN_TIMES:
			ui_run_times();
			break;
		case FUNCTION_CALENDAR:
			ui_calendar();
			break;
		case FUNCTION_SEASONAL_ADJUSTMENT:
			ui_seasonal_adjustment();
			break;
		case FUNCTION_SOLAR_SYNC:
			ui_solar_sync();
			break;
		case FUNCTION_MANUAL:
			ui_manual();
			break;
		case FUNCTION_OFF:
			ui_off();
			break;
		case FUNCTION_START_STATIONS:
			ui_start_stations();
			break;
		case FUNCTION_PROGRESS:
			ui_progress();
			break;
	}

	if (selection == FUNCTION_RUN || selection == FUNCTION_OFF || selection == FUNCTION_PROGRESS)
	{
		// blink CYCLE icon is no AC is sensed
		if (!ac_sensed && !(ticks_fractions & 4))
			display_set_other_icons(ICON_CYCLE);

		if (rain_sensed)
			display_set_other_icons(ICON_UMBRELLA | ICON_SPRAY | ICON_SPRAY_STOP);

		uint8_t stations_mask = stations_queue_mask();
		if (stations_mask)
		{
			display_drops(stations_mask, 0);

			if (!(ticks_fractions & 4))
				display_set_other_icons(ICON_SPRAY);
		}
	}

	display_update();
}

void ui_run(void)
{
	display_digit(2, now.hours >> 4);
	display_digit(3, now.hours & 15);
	display_digit(4, now.minutes >> 4);
	display_digit(5, now.minutes & 15);

	display_weekdays((uint8_t)(1 << now.weekday));

	if (!RTCCFGbits.HALFSEC)
		display_set_calendar_icons(ICON_TIME_COMMA);

	uint16_t bars = 1;
	for (uint8_t n = 1; n < programs_seasonal_adjustment; ++n)
		bars = (bars << 1) | 1;

	display_bars(bars);
}

void ui_current_time(void)
{
	if (buttons & (LEFT | RIGHT))
		selection_tab = update_number(selection_tab, 0, 4, (buttons & RIGHT));

	if (buttons & (PLUS | MINUS))
	{
		switch (selection_tab)
		{
			case 0:
				now.year = update_bcd(now.year, 0x00, 0x99, (buttons & PLUS));
				break;
			case 1:

				now.day = update_bcd(now.day, 0x01, rtcc_month_days(&now), (buttons & PLUS));
				break;
			case 2:
				now.month = update_bcd(now.month, 0x01, 0x12, (buttons & PLUS));
				break;
			case 3:
				now.hours = update_bcd(now.hours, 0x00, 0x23, (buttons & PLUS));
				break;
			case 4:
				now.minutes = update_bcd(now.minutes, 0x00, 0x59, (buttons & PLUS));
				break;
		}

		rtcc_fix(&now);
		rtcc_set(&now, true);

		blink_delay = 8;
	}

	bool blink = in_blink();

	switch (selection_tab)
	{
		case 0:
			display_weekdays((uint8_t)(1 << now.weekday));

			if (blink)
				break;
			display_digit(2, 2);
			display_digit(3, 0);
			display_digit(4, now.year >> 4);
			display_digit(5, now.year & 15);
			break;
		case 1:
		case 2:
			if (selection_tab == 2)
				display_set_calendar_icons(ICON_DATE_MONTH);
			else
				display_set_calendar_icons(ICON_DATE_DAY);

			display_weekdays((uint8_t)(1 << now.weekday));

			if (selection_tab == 2 || !blink)
			{
				display_digit(4, now.day >> 4);
				display_digit(5, now.day & 15);
			}
			if (selection_tab == 1 || !blink)
			{
				display_digit(2, now.month >> 4);
				display_digit(3, now.month & 15);
			}
			break;
		case 3:
		case 4:
			display_set_calendar_icons(ICON_TIME_COMMA);

			if (selection_tab == 4 || !blink)
			{
				display_digit(2, now.hours >> 4);
				display_digit(3, now.hours & 15);
			}
			if (selection_tab == 3 || !blink)
			{
				display_digit(4, now.minutes >> 4);
				display_digit(5, now.minutes & 15);
			}
			break;
	}
}

void ui_start_times(void)
{
	if (buttons & PRG)
		if (++program_number == NUMBER_OF_PROGRAMS)
			program_number = 0;

	if (buttons & (LEFT | RIGHT))
		selection_tab ^= 1;

	program_start_time_t* start_time = &(programs[program_number].start_time);

	if (buttons & (PLUS | MINUS))
	{
		switch (selection_tab)
		{
			case 0:
				start_time->hour = update_bcd(start_time->hour, 0x00, 0x24, (buttons & PLUS));
				break;
			case 1:
				start_time->minute = update_bcd(start_time->minute, 0x00, 0x59, (buttons & PLUS));
				break;
		}

		programs_changed = true;

		blink_delay = 8;
	}

	bool blink = in_blink();

	display_set_other_icons(ICON_ALARM_CLOCK);

	display_digit(0, program_number + 1);

	if (start_time->hour == 0x24)
	{
		// OFF
		if (!blink)
		{
			display_digit_segments(3, 0b01110111);
			display_digit_segments(4, 0b00011011);
			display_digit_segments(5, 0b00011011);
		}
	}
	else
	{
		display_set_calendar_icons(ICON_TIME_COMMA);

		if (selection_tab == 1 || !blink)
		{
			display_digit(2, start_time->hour >> 4);
			display_digit(3, start_time->hour & 15);
		}
		if (selection_tab == 0 || !blink)
		{
			display_digit(4, start_time->minute >> 4);
			display_digit(5, start_time->minute & 15);
		}
	}
}

void ui_run_times(void)
{
	if (buttons & PRG)
		if (++program_number == NUMBER_OF_PROGRAMS)
			program_number = 0;

	if (buttons & (LEFT | RIGHT))
		selection_tab = update_number(selection_tab, 0, NUMBER_OF_STATIONS - 1, (buttons & RIGHT));

	uint8_t run_time = programs[program_number].run_times[selection_tab];

	if (buttons & (PLUS | MINUS))
	{
		run_time = update_number(run_time, 0, 240, (buttons & PLUS));

		programs[program_number].run_times[selection_tab] = run_time;
		programs_changed = true;

		blink_delay = 8;
	}

	display_set_other_icons(ICON_SAND_CLOCK);

	display_digit(0, program_number + 1);
	display_digit(1, selection_tab + 1);

	if (!in_blink())
	{
		display_set_calendar_icons(ICON_TIME_COMMA);

		uint8_t hours = number_to_bcd(run_time / 60);
		uint8_t minutes = number_to_bcd(run_time % 60);
		display_digit(3, hours & 15);
		display_digit(4, minutes >> 4);
		display_digit(5, minutes & 15);
	}
}

void ui_calendar(void)
{
	if (buttons & PRG)
	{
		if (++program_number == NUMBER_OF_PROGRAMS)
			program_number = 0;

		selection_tab = 0;
	}

	program_calendar_t* calendar = &(programs[program_number].calendar);

	if (selection_tab == 0)
	{
		// tab == 0 is initial state, proper tab need to be set according to calendar data
		if (calendar->repeat_bits == 0b10)
			selection_tab = 8;
		else if (calendar->odd_even_bits == 0b110)
			selection_tab = 9;
		else
			selection_tab = 1;
	}

	if (buttons & (LEFT | RIGHT))
	{
		selection_tab = update_number(selection_tab, 1, 9, (buttons & RIGHT));

		if (selection_tab == 8 && calendar->repeat_bits != 0b10)
			// set default day repeat values
			calendar->repeat_bits = 0b10, calendar->days = 0, calendar->offset = 0;
		else if (selection_tab == 9 && calendar->odd_even_bits != 0b110)
			// set default odd/even values
			calendar->odd_even_bits = 0b110, calendar->odd_even = 0;
		else if (calendar->weekdays_bit != 0b0)
			// set default weekdays values
			calendar->weekdays_bit = 0b0, calendar->weekdays_mask = 0b1111111;
	}

	if (buttons & (PLUS | MINUS))
	{
		switch (selection_tab)
		{
			case 8:
				// skip days
				calendar->days = update_number(calendar->days, 0, 7, (buttons & PLUS));
				{
					datetime_t now;
					rtcc_get(&now);

					// if programmed start time already passed today, next day is planned
					bool today_passed = false;
					program_start_time_t start_time = programs[program_number].start_time;
					if (now.hours > start_time.hour || (now.hours == start_time.hour && now.minutes > start_time.minute))
						today_passed = true;

					calendar->offset = (rtcc_year_day(&now) + today_passed) % (calendar->days + 1);
				}
				break;
			case 9:
				// odd/even
				calendar->odd_even = !calendar->odd_even;
				break;
			default:
				// weekdays
				calendar->weekdays_mask ^= (1 << (selection_tab - 1));
				break;
		}

		programs_changed = true;

		blink_delay = 8;
	}

	display_digit(0, program_number + 1);

	switch (selection_tab)
	{
		case 8:
			// skip day
			display_digit(5, calendar->days + 1);
			display_set_calendar_icons(ICON_DATE_DAY);
			break;
		case 9:
			// odd/even
			display_set_calendar_icons(calendar->odd_even ? ICON_DATE_ODD : ICON_DATE_EVEN);
			display_set_calendar_icons(ICON_DATE_DAY);
			break;
		default:
			// weekdays
			display_weekdays(0b01111111);

			if (in_blink())
				display_drops(0b01111111 & ~(1 << (selection_tab - 1)), (~calendar->weekdays_mask) & 0b01111111);
			else
				display_drops(0b01111111, (~calendar->weekdays_mask) & 0b01111111);
			break;
	}
}

void ui_seasonal_adjustment(void)
{
	if (buttons & (PLUS | MINUS))
	{
		programs_seasonal_adjustment = update_number(programs_seasonal_adjustment, 1, 15, (buttons & PLUS));
		programs_changed = true;
	}

	uint16_t bars = 1;
	for (uint8_t n = 1; n < programs_seasonal_adjustment; ++n)
		bars = (bars << 1) | 1;

	display_set_other_icons(ICON_PERCENT);
	display_bars(bars);
	if (programs_seasonal_adjustment > 9)
		display_digit(3, 1);
	display_digit(4, programs_seasonal_adjustment % 10);
	display_digit(5, 0);
}

void ui_solar_sync(void)
{
}

void ui_manual(void)
{
}

void ui_off(void)
{
	if (selection_tab > 16 || !in_blink())
		display_set_other_icons(ICON_SPRAY | ICON_SPRAY_STOP);

	// wait a two seconds (16x125ms frames) before actually initiating OFF function to allow user
	// to rotate controller through this function to another one without stopping the watering
	if (selection_tab < 16)
	{
		selection_tab++;
		return;
	}
	if (selection_tab == 16)
	{
		selection_tab++;
		stations_queue_stop();
	}

	display_digit_segments(3, 0b01110111);
	display_digit_segments(4, 0b00011011);
	display_digit_segments(5, 0b00011011);
}

void ui_start_stations(void)
{
	display_set_other_icons(ICON_SAND_CLOCK);

	static uint8_t run_times[NUMBER_OF_STATIONS];
	static uint8_t program_number;

	if (selection_tab == 0)
	{
		// wait for button RIGHT being un-pressed
		if (buttons == RIGHT && buttons_repeat > 0)
			return;

		for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n)
			run_times[n] = 0;

		program_number = 0;
		selection_tab = 1;
	}

	// start stations when no buttons was pressed in 5 seconds
	if (idle_time > 30)
	{
		bool any_started = false;
		for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n)
			if (stations_queue_start(n, run_times[n] * 60))
				any_started = true;

		// TODO: reset calendar offset if whole program is queued?

		// show watering progress or fall back to previous selection
		ui_change_selection(any_started ? FUNCTION_PROGRESS : selection_previous);
		return;
	}

	if (buttons & PRG)
	{
		program_number = update_number(program_number, 1, NUMBER_OF_PROGRAMS, true);

		for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n)
		{
			uint16_t run_time = programs[program_number - 1].run_times[n];
			run_time *= programs_seasonal_adjustment;
			run_time /= 10;
			run_times[n] = (uint8_t)run_time;
		}
	}

	if (buttons & (LEFT | RIGHT))
		selection_tab = update_number(selection_tab, 1, NUMBER_OF_STATIONS, (buttons & RIGHT));

	uint8_t run_time = run_times[selection_tab - 1];

	if (buttons & (PLUS | MINUS))
	{
		run_time = update_number(run_time, 0, 240, (buttons & PLUS));
		run_times[selection_tab - 1] = run_time;

		program_number = 0;

		blink_delay = 8;
	}

	if (program_number)
		display_digit(0, program_number);

	display_digit(1, selection_tab);

	if (!in_blink())
	{
		display_set_calendar_icons(ICON_TIME_COMMA);

		uint8_t hours = number_to_bcd(run_time / 60);
		uint8_t minutes = number_to_bcd(run_time % 60);
		display_digit(3, hours & 15);
		display_digit(4, minutes >> 4);
		display_digit(5, minutes & 15);
	}
}

void ui_progress(void)
{
	uint16_t most_recent_run_time;
	if (!stations_queue_progress(&most_recent_run_time))
		// no station is running anymore
		return ui_change_selection(selection_previous);

	// show progress on display
	if (!(ticks_fractions & 4))
		display_set_calendar_icons(ICON_TIME_COMMA);

	uint8_t hours = number_to_bcd(most_recent_run_time / 3600);
	uint8_t minutes = number_to_bcd((most_recent_run_time % 3600) / 60);
	uint8_t seconds = number_to_bcd((most_recent_run_time % 3600) % 60);
	if (hours > 0)
		display_digit(1, hours & 15);
	display_digit(2, minutes >> 4);
	display_digit(3, minutes & 15);
	display_digit(4, seconds >> 4);
	display_digit(5, seconds & 15);
}

uint8_t ui_selection(void)
{
	return selection;
}

void ui_change_selection(uint8_t number)
{
	// change the selection and reset its tab and frame
	selection = number;

	if (number <= FUNCTION_OFF)
		selection_previous = selection;

	selection_tab = 0;
}

static bool in_blink(void)
{
	bool blink = false;
	if (blink_delay > 0)
		blink_delay--;
	else
		blink = (ticks_fractions & 4);

	return blink;
}

static void update_buttons(void)
{
	static uint8_t buttons_previous = 0;

	buttons = controls_buttons();
	if (buttons == buttons_previous)
	{
		if (buttons_repeat < 255)
			buttons_repeat++;
		if (buttons_repeat < 3)
			buttons = 0;
	}
	else
	{
		buttons_previous = buttons;
		buttons_repeat = 0;
	}
}

static uint8_t update_number(uint8_t number, uint8_t min, uint8_t max, bool increase)
{
	if (increase)
		return (number >= max) ? min : number + 1;
	else
		return (number <= min) ? max : number - 1;
}

static uint8_t update_bcd(uint8_t number, uint8_t min, uint8_t max, bool increase)
{
	if (increase)
		return (number >= max) ? min : ((number & 0x0F) < 9) ? (number + 1) : ((number & 0xF0) + 0x10);
	else
		return (number <= min) ? max : ((number & 0x0F) > 0) ? (number - 1) : ((number & 0xF0) - 0x10 + 0x09);
}