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

#include "remote.h"
#include "programs.h"
#include "stations.h"
#include "controls.h"
#include "ui.h"
#include "rtcc.h"

static uint8_t packet[32];

static uint8_t crc_update(uint8_t crc, uint8_t data)
{
	data ^= crc;
	crc = 0;

	if (data & 0x01)
		crc ^= 0x5e;
	if (data & 0x02)
		crc ^= 0xbc;
	if (data & 0x04)
		crc ^= 0x61;
	if (data & 0x08)
		crc ^= 0xc2;
	if (data & 0x10)
		crc ^= 0x9d;
	if (data & 0x20)
		crc ^= 0x23;
	if (data & 0x40)
		crc ^= 0x46;
	if (data & 0x80)
		crc ^= 0x8c;

	return crc;
}

static bool wait_clock(uint8_t prescaler, uint8_t ticks, bool state)
{
	// wait for clock pulse flips to state, no longer than 200us
	T0CON = 0b11000000 + prescaler;
	TMR0L = 255 - ticks + 1;
	INTCONbits.T0IF = 0;
	while (!INTCONbits.T0IF && PORTBbits.PGC != state);
	return !INTCONbits.T0IF;
}

static uint8_t receive_packet(void)
{
	// remote input is signaled by HIGH on PGC
	if (!PORTBbits.PGC)
		return 0;

	// signal remote on PGD to send its accept signal
	TRISBbits.TRISB7 = 0;
	PORTBbits.PGD = 1;
	// wait for remote to accept signal by setting PGC to LOW, no longer than 5ms
	bool timed_out = !wait_clock(7, 20, false);

	// switch PGD back to input
	PORTBbits.PGD = 0;
	TRISBbits.TRISB7 = 1;

	if (timed_out)
		// wait for accept signal timed out
		return 0;

	// read data from PGD, single bit a PGC pulse
	uint8_t packet_len = 0;
	for (uint8_t bytes = 0; bytes < sizeof(packet); ++bytes)
	{
		packet[bytes] = 0;

		uint8_t mask = 1;
		for (uint8_t bits = 0; bits < 8; ++bits, mask <<= 1)
		{
			// wait for clock pulse flips to HIGH, no longer than 200us
			if (!wait_clock(0, 100, true))
			{
				if (bits > 0)
					// no clock in middle of byte means error
					return 0;

				// no clock after entire byte means end of packet

				if (packet_len < 2)
					// packet must contain at least single byte payload followed by 8bit CRC
					return 0;

				// check packet CRC
				uint8_t crc = 0;
				for (uint8_t n = 0; n < packet_len - 1; ++n)
					crc = crc_update(crc, packet[n]);
				if (packet[packet_len - 1] != crc)
					// packet CRC mismatch!
					return 0;

				// return size of payload without CRC at the end
				return packet_len - 1;
			}

			if (PORTBbits.PGD)
				packet[bytes] |= mask;

			// wait for clock pulse flips to LOW, no longer than 200us
			if (!wait_clock(0, 100, false))
				return 0;
		}

		if (packet_len++ == UINT8_MAX)
			break;
	}

	// packet is too long
	return 0;
}

static bool send_byte(uint8_t data)
{
	uint8_t mask = 1;
	for (uint8_t bits = 0; bits < 8; ++bits, mask <<= 1)
	{
		PORTBbits.PGD = (data & mask) ? 1 : 0;

		// wait for clock pulse flips to HIGH, no longer than 200us
		if (!wait_clock(0, 100, true))
			return false;

		// wait for clock pulse flips to LOW, no longer than 200us
		if (!wait_clock(0, 100, false))
			return false;
	}

	return true;
}

static void send_start()
{
	// signal the remote that some data are going to be sent
	TRISBbits.TRISB7 = 0;
	PORTBbits.PGD = 1;
}

static void send_finish()
{
	// switch PGD back to input
	PORTBbits.PGD = 0;
	TRISBbits.TRISB7 = 1;
}

static bool send_packet(const uint8_t* data, size_t length)
{
	if (length > UINT8_MAX)
		return false;

	// compute packet CRC
	uint8_t crc = 0;
	for (uint8_t n = 0; n < length; ++n)
		crc = crc_update(crc, data[n]);

	// PGD should be already set to HIGH to signal remote that some data are going to be sent

	// remote accepts it by setting PGC to HIGH, wait no longer than 5ms
	bool timed_out = !wait_clock(7, 20, true);

	PORTBbits.PGD = 0;

	if (timed_out)
		// wait for accept signal timed out
		return false;

	// wait for PGC pulse to going LOW before sending the data, no longer than 5ms
	if (!wait_clock(7, 20, false))
		return false;

	// write data to PGD, single bit a PGC pulse
	if (!send_byte((uint8_t)length))
		return false;

	for (uint8_t n = 0; n < length; ++n)
		if (!send_byte(data[n]))
			return false;

	if (!send_byte(crc))
		return false;

	return true;
}

void remote_init(void)
{
	// PGC/RB6 clock input
	TRISBbits.TRISB6 = 1;

	// PGD/RB7 data input/output (input initially)
	TRISBbits.TRISB7 = 1;
}

void remote_handle(void)
{
	uint8_t packet_len = receive_packet();
	if (packet_len == 0)
		// no valid packet received
		return;

	switch (packet[0])
	{
		case 0xA0:
		{
			// start program
			if (packet_len != 2 || packet[1] >= NUMBER_OF_PROGRAMS)
				return;

			if (programs_queue(&programs[packet[1]]))
				ui_change_selection(FUNCTION_PROGRESS);

			break;
		}

		case 0xA1:
		{
			// start stations
			bool any_started = false;
			for (uint8_t n = 0; n < packet_len - 1 && n < NUMBER_OF_STATIONS; ++n)
				if (stations_queue_start(n, packet[n + 1] * 60))
					any_started = true;

			if (any_started)
				ui_change_selection(FUNCTION_PROGRESS);

			break;
		}

		case 0xA2:
		{
			// stop all
			stations_queue_stop();
			break;
		}

		case 0xA3:
		{
			// change seasonal adjustment
			if (packet_len != 2 || (packet[1] < 1 || packet[1] > 15))
				return;

			programs_seasonal_adjustment = packet[1];
			programs_save();
			break;
		}

		case 0xA4:
		{
			// set date/time
			if (packet_len != 6)
				return;

			now.year = number_to_bcd(packet[1]);
			now.month = number_to_bcd(packet[2]);
			now.day = number_to_bcd(packet[3]);
			now.hours = number_to_bcd(packet[4]);
			now.minutes = number_to_bcd(packet[5]);

			rtcc_fix(&now);
			rtcc_set(&now, true);
			rtcc_sync();
			break;
		}

		case 0xB0:
		{
			// prepare reset
			// close all stations and wait a second for external reset to ensure it is safe (i.e. no EEPROM write is performed)
			stations_close_all();
			__delay_ms(1000);
		}

		case 0xB1:
		{
			// get unit info

			// signal remote that it should wait for data, preparing the data packet may take some time
			send_start();

			// prepare the data packet
			struct
			{
				struct
				{
					uint8_t day;
					uint8_t month;
					uint8_t year;
					uint8_t hours;
					uint8_t minutes;
				} datetime;
				uint8_t seasonal_adjustment;
				uint16_t stations[NUMBER_OF_STATIONS];
			}
			packet;

			packet.datetime.day = bcd_to_number(now.day);
			packet.datetime.month = bcd_to_number(now.month);
			packet.datetime.year = bcd_to_number(now.year);
			packet.datetime.hours = bcd_to_number(now.hours);
			packet.datetime.minutes = bcd_to_number(now.minutes);

			packet.seasonal_adjustment = programs_seasonal_adjustment;

			extern station_state_t stations_states[NUMBER_OF_STATIONS];
			for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n)
				packet.stations[n] = stations_states[n].run_time;

			// send the data packet
			send_packet((const uint8_t*)&packet, sizeof(packet));
			send_finish();
		}
	}
}