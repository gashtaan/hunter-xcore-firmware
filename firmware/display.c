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

#include "display.h"

#define LCD_SEGMENT(address, bit) ((uint8_t)(address - 0xF66) | (bit << 5))
#define LCD_SEGMENT_NONE() 0xFF

static const uint8_t lcd_segments_table[] =
{
	// digit 0
	LCD_SEGMENT(0xF6A, 4),
	LCD_SEGMENT(0xF6A, 3),
	LCD_SEGMENT(0xF70, 4),
	LCD_SEGMENT(0xF70, 3),
	LCD_SEGMENT(0xF76, 3),
	LCD_SEGMENT(0xF76, 4),
	LCD_SEGMENT(0xF7C, 3),
	LCD_SEGMENT_NONE(),

	// digit 1
	LCD_SEGMENT(0xF6A, 2),
	LCD_SEGMENT(0xF6B, 4),
	LCD_SEGMENT(0xF70, 2),
	LCD_SEGMENT(0xF71, 4),
	LCD_SEGMENT(0xF77, 4),
	LCD_SEGMENT(0xF76, 2),
	LCD_SEGMENT(0xF7D, 4),
	LCD_SEGMENT_NONE(),

	// digit 2
	LCD_SEGMENT(0xF6A, 6),
	LCD_SEGMENT(0xF6A, 5),
	LCD_SEGMENT(0xF70, 6),
	LCD_SEGMENT(0xF70, 5),
	LCD_SEGMENT(0xF76, 5),
	LCD_SEGMENT(0xF76, 6),
	LCD_SEGMENT(0xF7C, 5),
	LCD_SEGMENT_NONE(),

	// digit 3
	LCD_SEGMENT(0xF68, 3),
	LCD_SEGMENT(0xF6A, 7),
	LCD_SEGMENT(0xF6E, 3),
	LCD_SEGMENT(0xF70, 7),
	LCD_SEGMENT(0xF76, 7),
	LCD_SEGMENT(0xF74, 3),
	LCD_SEGMENT(0xF7C, 7),
	LCD_SEGMENT_NONE(),

	// digit 4
	LCD_SEGMENT(0xF6B, 2),
	LCD_SEGMENT(0xF6B, 0),
	LCD_SEGMENT(0xF71, 2),
	LCD_SEGMENT(0xF71, 0),
	LCD_SEGMENT(0xF77, 0),
	LCD_SEGMENT(0xF77, 2),
	LCD_SEGMENT(0xF7D, 0),
	LCD_SEGMENT_NONE(),

	// digit 5
	LCD_SEGMENT(0xF68, 4),
	LCD_SEGMENT(0xF6B, 3),
	LCD_SEGMENT(0xF6E, 4),
	LCD_SEGMENT(0xF71, 3),
	LCD_SEGMENT(0xF77, 3),
	LCD_SEGMENT(0xF74, 4),
	LCD_SEGMENT(0xF7D, 3),
	LCD_SEGMENT_NONE(),

	// drops
	LCD_SEGMENT(0xF78, 1),
	LCD_SEGMENT(0xF78, 2),
	LCD_SEGMENT(0xF78, 3),
	LCD_SEGMENT(0xF78, 4),
	LCD_SEGMENT(0xF78, 5),
	LCD_SEGMENT(0xF78, 6),
	LCD_SEGMENT(0xF78, 7),
	LCD_SEGMENT(0xF7C, 1),

	// drop stops
	LCD_SEGMENT(0xF72, 1),
	LCD_SEGMENT(0xF72, 2),
	LCD_SEGMENT(0xF72, 3),
	LCD_SEGMENT(0xF72, 4),
	LCD_SEGMENT(0xF72, 5),
	LCD_SEGMENT(0xF72, 6),
	LCD_SEGMENT(0xF72, 7),
	LCD_SEGMENT(0xF76, 1),

	// weekdays
	LCD_SEGMENT(0xF6C, 1),
	LCD_SEGMENT(0xF6C, 2),
	LCD_SEGMENT(0xF6C, 3),
	LCD_SEGMENT(0xF6C, 4),
	LCD_SEGMENT(0xF6C, 5),
	LCD_SEGMENT(0xF6C, 6),
	LCD_SEGMENT(0xF6C, 7),
	LCD_SEGMENT_NONE(),

	// bars
	LCD_SEGMENT(0xF66, 0),
	LCD_SEGMENT(0xF6C, 0),
	LCD_SEGMENT(0xF72, 0),
	LCD_SEGMENT(0xF7D, 7),
	LCD_SEGMENT(0xF77, 7),
	LCD_SEGMENT(0xF71, 7),
	LCD_SEGMENT(0xF6B, 7),
	LCD_SEGMENT(0xF6B, 6),
	LCD_SEGMENT(0xF71, 6),
	LCD_SEGMENT(0xF77, 6),
	LCD_SEGMENT(0xF7D, 6),
	LCD_SEGMENT(0xF7D, 5),
	LCD_SEGMENT(0xF77, 5),
	LCD_SEGMENT(0xF71, 5),
	LCD_SEGMENT(0xF6B, 5),
	LCD_SEGMENT_NONE(),

	// time comma, 24H, AM, PM, calendar 1-12, calendar 1-31, odd, even
	LCD_SEGMENT(0xF7A, 3),
	LCD_SEGMENT(0xF6E, 6),
	LCD_SEGMENT(0xF68, 5),
	LCD_SEGMENT(0xF68, 6),
	LCD_SEGMENT(0xF7C, 4),
	LCD_SEGMENT(0xF7A, 4),
	LCD_SEGMENT(0xF70, 1),
	LCD_SEGMENT(0xF78, 0),

	// cycle, soak, umbrella, spray, spray stop, sand clock, alarm clock, percent
	LCD_SEGMENT(0xF7A, 5),
	LCD_SEGMENT(0xF7A, 6),
	LCD_SEGMENT(0xF7D, 2),
	LCD_SEGMENT(0xF74, 6),
	LCD_SEGMENT(0xF74, 5),
	LCD_SEGMENT(0xF7C, 6),
	LCD_SEGMENT(0xF7C, 2),
	LCD_SEGMENT(0xF6E, 5),
};

static union lcd_segments_map_t
{
	struct
	{
		uint8_t digit_0_b:1;
		uint8_t digit_0_br:1;
		uint8_t digit_0_bl:1;
		uint8_t digit_0_m:1;
		uint8_t digit_0_tr:1;
		uint8_t digit_0_tl:1;
		uint8_t digit_0_t:1;
		uint8_t :1;

		uint8_t digit_1_b:1;
		uint8_t digit_1_br:1;
		uint8_t digit_1_bl:1;
		uint8_t digit_1_m:1;
		uint8_t digit_1_tr:1;
		uint8_t digit_1_tl:1;
		uint8_t digit_1_t:1;
		uint8_t :1;

		uint8_t digit_2_b:1;
		uint8_t digit_2_br:1;
		uint8_t digit_2_bl:1;
		uint8_t digit_2_m:1;
		uint8_t digit_2_tr:1;
		uint8_t digit_2_tl:1;
		uint8_t digit_2_t:1;
		uint8_t :1;

		uint8_t digit_3_b:1;
		uint8_t digit_3_br:1;
		uint8_t digit_3_bl:1;
		uint8_t digit_3_m:1;
		uint8_t digit_3_tr:1;
		uint8_t digit_3_tl:1;
		uint8_t digit_3_t:1;
		uint8_t :1;

		uint8_t digit_4_b:1;
		uint8_t digit_4_br:1;
		uint8_t digit_4_bl:1;
		uint8_t digit_4_m:1;
		uint8_t digit_4_tr:1;
		uint8_t digit_4_tl:1;
		uint8_t digit_4_t:1;
		uint8_t :1;

		uint8_t digit_5_b:1;
		uint8_t digit_5_br:1;
		uint8_t digit_5_bl:1;
		uint8_t digit_5_m:1;
		uint8_t digit_5_tr:1;
		uint8_t digit_5_tl:1;
		uint8_t digit_5_t:1;
		uint8_t :1;

		uint8_t drop_1:1;
		uint8_t drop_2:1;
		uint8_t drop_3:1;
		uint8_t drop_4:1;
		uint8_t drop_5:1;
		uint8_t drop_6:1;
		uint8_t drop_7:1;
		uint8_t drop_8:1;

		uint8_t drop_stop_1:1;
		uint8_t drop_stop_2:1;
		uint8_t drop_stop_3:1;
		uint8_t drop_stop_4:1;
		uint8_t drop_stop_5:1;
		uint8_t drop_stop_6:1;
		uint8_t drop_stop_7:1;
		uint8_t drop_stop_8:1;

		uint8_t weekday_mo:1;
		uint8_t weekday_tu:1;
		uint8_t weekday_we:1;
		uint8_t weekday_th:1;
		uint8_t weekday_fr:1;
		uint8_t weekday_sa:1;
		uint8_t weekday_su:1;
		uint8_t :1;

		uint8_t bar_1:1;
		uint8_t bar_2:1;
		uint8_t bar_3:1;
		uint8_t bar_4:1;
		uint8_t bar_5:1;
		uint8_t bar_6:1;
		uint8_t bar_7:1;
		uint8_t bar_8:1;

		uint8_t bar_9:1;
		uint8_t bar_10:1;
		uint8_t bar_11:1;
		uint8_t bar_12:1;
		uint8_t bar_13:1;
		uint8_t bar_14:1;
		uint8_t bar_15:1;
		uint8_t :1;

		uint8_t time_comma:1;
		uint8_t time_24hr:1;
		uint8_t time_am:1;
		uint8_t time_pm:1;
		uint8_t calendar_month:1;
		uint8_t calendar_day:1;
		uint8_t odd:1;
		uint8_t even:1;

		uint8_t cycle:1;
		uint8_t soak:1;
		uint8_t umbrella:1;
		uint8_t spray:1;
		uint8_t spray_stop:1;
		uint8_t sand_clock:1;
		uint8_t alarm_clock:1;
		uint8_t percent:1;
	};
	struct
	{
		uint8_t digit_0;
		uint8_t digit_1;
		uint8_t digit_2;
		uint8_t digit_3;
		uint8_t digit_4;
		uint8_t digit_5;
		uint8_t drops;
		uint8_t drops_stop;
		uint8_t weekdays;
		uint16_t bars;
		uint8_t calendar_icons;
		uint8_t other_icons;
	};
}
lcd_segments_map = {};

void display_init(void)
{
	// LCDEN = 1, SLPEN = 1, CS = 0b11, LMUX = 0b11
	LCDCON = 0b11001111;

	// LCDA = 1, WA = 1
	LCDPS = 0b00110000;

	// LCDIRE = 1, LCDIRS = 1, LCDCST = 0b000 (maximum contrast)
	LCDREF = 0b11000000;

	// LRLAP = 0b11, LRLBP = 0b11
	LCDRL = 0b11110000;

	// enable segments
	LCDSE0 = 0b11111111;
	LCDSE1 = 0b00000000;
	LCDSE2 = 0b01111000;
	LCDSE3 = 0b00000000;
	LCDSE4 = 0b11111110;
	LCDSE5 = 0b11111101;

	display_all();
	display_update();
	__delay_ms(500);
	display_clear();
}

void display_update(void)
{
	uint8_t* map_address = (uint8_t*)&lcd_segments_map;
	for (char n = 0; n < sizeof(lcd_segments_table); ++n)
	{
		uint8_t segment = lcd_segments_table[n];
		if (segment == 0xFF)
			continue;

		uint8_t* address = (uint8_t*)(0xF66 + (segment & 0x1F));
		uint8_t mask = (uint8_t)(1 << (segment >> 5));

		if (map_address[n >> 3] & (1 << (n & 0x07)))
			*address |= mask;
		else
			*address &= ~mask;
	}
}

void display_clear(void)
{
	for (unsigned short n = 0; n < sizeof(lcd_segments_map); ++n)
		((uint8_t*)&lcd_segments_map)[n] = 0;
}

void display_all(void)
{
	for (unsigned short n = 0; n < sizeof(lcd_segments_map); ++n)
		((uint8_t*)&lcd_segments_map)[n] = 0xFF;
}

void display_digit(uint8_t position, uint8_t number)
{
	uint8_t bits = 0;
	switch (number)
	{
		// b, br, bl, m, tr, tl, t
		case 0:
			bits = 0b01110111;
			break;
		case 1:
			bits = 0b00100100;
			break;
		case 2:
			bits = 0b01011101;
			break;
		case 3:
			bits = 0b01101101;
			break;
		case 4:
			bits = 0b00101110;
			break;
		case 5:
			bits = 0b01101011;
			break;
		case 6:
			bits = 0b01111011;
			break;
		case 7:
			bits = 0b00100101;
			break;
		case 8:
			bits = 0b01111111;
			break;
		case 9:
			bits = 0b01101111;
			break;
	}
	display_digit_segments(position, bits);
}
void display_digit_segments(uint8_t position, uint8_t bits)
{
	(&lcd_segments_map.digit_0)[position] = bits;
}

void display_bars(unsigned short bits)
{
	lcd_segments_map.bars = bits;
}

void display_weekdays(uint8_t bits)
{
	lcd_segments_map.weekdays = bits;
}

void display_drops(uint8_t bits, uint8_t bits_stops)
{
	lcd_segments_map.drops = bits;
	lcd_segments_map.drops_stop = bits_stops;
}

void display_set_calendar_icons(uint8_t mask)
{
	lcd_segments_map.calendar_icons |= mask;
}
void display_set_other_icons(uint8_t mask)
{
	lcd_segments_map.other_icons |= mask;
}