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

#include <Arduino.h>
#include "io.h"

#ifdef ARDUINO_ESP8266_WEMOS_D1MINILITE
	// WeMos D1 Mini
	const uint8_t RST = 16;	// D0
	const uint8_t PGC = 5;	// D1
	const uint8_t PGD = 4;	// D2
#else
	// ESP-01
	const uint8_t RST = 1;	// TX
	const uint8_t PGC = 2;	// GPIO02
	const uint8_t PGD = 3;	// RX
#endif

void io_init()
{
	pinMode(RST, OUTPUT);
	pinMode(PGC, OUTPUT);
	pinMode(PGD, OUTPUT);

	io_reset(HIGH);
	io_clock(LOW);
	io_data(LOW);
}

void io_reset(bool level)
{
	digitalWrite(RST, level);
}

void io_clock(bool level)
{
	digitalWrite(PGC, !level);
}

void io_data(bool level)
{
	digitalWrite(PGD, !level);
}
bool io_data()
{
	return !digitalRead(PGD);
}

void io_mode(int mode)
{
	pinMode(PGD, mode);
}

bool io_receiveBit()
{
	delayMicroseconds(25);
	io_clock(HIGH);
	delayMicroseconds(50);
	bool value = io_data();
	io_clock(LOW);
	delayMicroseconds(25);
	return value;
}

void io_emitBit(bool value)
{
	io_data(value);
	delayMicroseconds(25);
	io_clock(HIGH);
	delayMicroseconds(50);
	io_clock(LOW);
	delayMicroseconds(25);
}

void io_emitPulse(unsigned int high, unsigned int low)
{
	io_clock(HIGH);
	delayMicroseconds(high);
	io_clock(LOW);
	delayMicroseconds(low);
}
