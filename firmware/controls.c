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

#include "controls.h"

void controls_init(void)
{
	PADCFG1bits.REPU = 1;
	TRISB &= ~0b00001110;
	TRISE |= 0b00000111;
	TRISA |= 0b00010110;
	TRISF |= 0b10000000;
	TRISG |= 0b00010000;
}

uint8_t controls_selection(void)
{
	uint8_t bits;

	PORTBbits.RB1 = 1;
	PORTBbits.RB2 = 1;
	PORTBbits.RB3 = 0;

	bits = PORTE & 7;
	if (bits == 0b110)
		return FUNCTION_RUN;
	if (bits == 0b101)
		return FUNCTION_CURRENT_TIME;
	if (bits == 0b011)
		return FUNCTION_START_TIMES;

	PORTBbits.RB1 = 1;
	PORTBbits.RB2 = 0;
	PORTBbits.RB3 = 1;

	bits = PORTE & 7;
	if (bits == 0b110)
		return FUNCTION_RUN_TIMES;
	if (bits == 0b101)
		return FUNCTION_CALENDAR;
	if (bits == 0b011)
		return FUNCTION_SEASONAL_ADJUSTMENT;

	PORTBbits.RB1 = 0;
	PORTBbits.RB2 = 1;
	PORTBbits.RB3 = 1;

	bits = PORTE & 7;
	if (bits == 0b110)
		return FUNCTION_SOLAR_SYNC;
	if (bits == 0b101)
		return FUNCTION_MANUAL;
	if (bits == 0b011)
		return FUNCTION_OFF;

	return FUNCTION_NONE;
}

uint8_t controls_buttons(void)
{
	PORTBbits.RB1 = 1;

	uint8_t bits = PORTA & 0b00010110;
	bits |= ((PORTF & 0b10000000) ? 8 : 0);
	bits |= ((PORTG & 0b00010000) ? 1 : 0);

	return bits;
}