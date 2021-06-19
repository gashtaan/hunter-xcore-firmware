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

#include "eeprom.h"

uint8_t eeprom_read_byte(uint16_t offset)
{
	EEADRH = offset >> 8;
	EEADR = offset & 255;

	EECON1bits.EEPGD = 0;
	EECON1bits.CFGS = 0;
	EECON1bits.RD = 1;
	NOP();

	return EEDATA;
}

void eeprom_write_byte(uint16_t offset, uint8_t byte)
{
	EEADRH = offset >> 8;
	EEADR = offset & 255;

	EEDATA = byte;

	EECON1bits.EEPGD = 0;
	EECON1bits.CFGS = 0;
	EECON1bits.WREN = 1;
	bool gie = INTCONbits.GIE;

	EECON2 = 0x55;
	EECON2 = 0xAA;

	EECON1bits.WR = 1;
	while(EECON1bits.WR);

	INTCONbits.GIE = gie;
	EECON1bits.WREN = 0;
}

void eeprom_read_data(uint8_t* ptr, uint16_t size, uint16_t offset)
{
	for (uint16_t n = 0; n < size; ++n)
		ptr[n] = eeprom_read_byte(offset + n);
}

void eeprom_write_data(const uint8_t* ptr, uint16_t size, uint16_t offset)
{
	for (uint16_t n = 0; n < size; ++n)
		eeprom_write_byte(offset + n, ptr[n]);
}

void eeprom_validate(void)
{
	eeprom_write_byte(0x3FF, 0xAA);
}
bool eeprom_check(void)
{
	return (eeprom_read_byte(0x3FF) == 0xAA);
}