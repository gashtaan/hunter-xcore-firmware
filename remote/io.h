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

void io_init();

void io_reset(bool level);

void io_data(bool level);
bool io_data();

void io_clock(bool level);

void io_mode(int mode);

bool io_receiveBit();
void io_emitBit(bool value);
void io_emitPulse(unsigned int high, unsigned int low);
