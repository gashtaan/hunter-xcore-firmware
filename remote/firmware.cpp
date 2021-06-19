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

void emitCommand(uint8_t command)
{
	for (size_t n = 0; n < 4; ++n)
		io_emitBit(command & (1 << n));
}

void emitData(uint16_t data)
{
	for (size_t n = 0; n < 16; ++n)
		io_emitBit(data & (1 << n));
}

uint8_t receiveData()
{
	for (size_t n = 0; n < 8; ++n)
		io_emitBit(LOW);

	io_mode(INPUT);
	delayMicroseconds(100);

	uint8_t data = 0;
	for (size_t n = 0; n < 8; ++n)
		data |= (io_receiveBit() ? (1 << n) : 0);

	io_mode(OUTPUT);
	delayMicroseconds(100);

	return data;
}

void loadAddress(uint32_t address)
{
	emitCommand(0b0000);
	emitData(0x0E00 + ((address >> 16) & 0xFF));
	emitCommand(0b0000);
	emitData(0x6EF8);
	emitCommand(0b0000);
	emitData(0x0E00 + ((address >> 8) & 0xFF));
	emitCommand(0b0000);
	emitData(0x6EF7);
	emitCommand(0b0000);
	emitData(0x0E00 + (address & 0xFF));
	emitCommand(0b0000);
	emitData(0x6EF6);
}

void loadWriteBuffer(uint16_t data, bool flushWrite)
{
	emitCommand(flushWrite ? 0b1111 : 0b1101);
	emitData(data);

	if (flushWrite)
	{
		// set 0b0000 command, but with prolonged its last bit clock cycle
		io_emitBit(0);
		io_emitBit(0);
		io_emitBit(0);
		io_emitPulse(1000, 200);

		emitData(0x0000);
	}
}

void blockErase(uint32_t section)
{
	auto b2w = [](uint8_t b) { return b | (b << 8); };

	loadAddress(0x3C0004);
	emitCommand(0b1100);
	emitData(b2w(section & 0x0000FF));

	loadAddress(0x3C0005);
	emitCommand(0b1100);
	emitData(b2w((section & 0x00FF00) >> 8));

	loadAddress(0x3C0006);
	emitCommand(0b1100);
	emitData(b2w((section & 0xFF0000) >> 16));

	emitCommand(0b0000);
	emitData(0x0000);

	emitCommand(0b0000);
	delayMicroseconds(5000);
	emitData(0x0000);

	yield();
}

void enterProgrammingMode()
{
	// enter low-voltage programming mode
	io_reset(LOW);
	delayMicroseconds(250);

	// send programming magic
	uint32_t magic = 0x4D434850;
	for (size_t n = 0; n < 32; ++n)
		io_emitBit(magic & (1 << (31 - n)));

	io_reset(HIGH);
}

uint16_t firmwareMcuId()
{
	enterProgrammingMode();

	uint16_t id = 0;

	loadAddress(0x3FFFFE);

	emitCommand(0b1001);
	id |= uint16_t(receiveData());

	emitCommand(0b1001);
	id |= uint16_t(receiveData()) << 8;

	return id;
}

bool firmwareUpload(const uint8_t* data, size_t dataSize)
{
	enterProgrammingMode();

	// erase flash memory 0000-FFFF (boot, block 0-3)
	blockErase(0x800005);
	blockErase(0x800104);
	blockErase(0x800204);
	blockErase(0x800404);
	blockErase(0x800804);

	// select program memory
	emitCommand(0b0000);
	emitData(0x8EA6);
	emitCommand(0b0000);
	emitData(0x9CA6);

	// upload program
	auto wdata = reinterpret_cast<const uint16_t*>(data);
	auto wdata_size = dataSize / 2;

	constexpr size_t WRITE_BUFFER_SIZE = 32;
	if (wdata_size % WRITE_BUFFER_SIZE)
		// data size has to be aligned to write buffer size
		return false;

	for (size_t n = 0; n < wdata_size; ++n)
	{
		auto mod = n % WRITE_BUFFER_SIZE;

		if (mod == 0)
			loadAddress(n * 2);

		loadWriteBuffer(wdata[n], mod == (WRITE_BUFFER_SIZE - 1));

		yield();
	}

	io_reset(LOW);
	delayMicroseconds(250);
	io_reset(HIGH);

	return true;
}
