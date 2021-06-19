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

#include "sensor.h"

void sensor_init(void)
{
	ANCON1 = 0;

	// output - enable(0)/disable(1) sensor 24V voltage
	TRISAbits.TRISA0 = 0;
	PORTAbits.RA0 = 1;

	// input - rain sensor on SEN terminals
	TRISGbits.TRISG0 = 1;

	// input - sensor bypass switch
	TRISAbits.TRISA3 = 1;
	TRISJbits.TRISJ0 = 0;
	PORTJbits.RJ0 = 1;
}

bool sensor_check(void)
{
	// rain sensor bypassed
	if (PORTAbits.RA3)
		return false;

	PORTAbits.RA0 = 0;
	__delay_ms(15); // wait for capacitor C113 to charge
	bool b = PORTGbits.RG3;
	PORTAbits.RA0 = 1;

	return b;
}