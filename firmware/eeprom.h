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

uint8_t eeprom_read_byte(uint16_t offset);
void eeprom_write_byte(uint16_t offset, uint8_t byte);
void eeprom_read_data(uint8_t* ptr, uint16_t size, uint16_t offset);
void eeprom_write_data(const uint8_t* ptr, uint16_t size, uint16_t offset);

void eeprom_validate(void);
bool eeprom_check(void);