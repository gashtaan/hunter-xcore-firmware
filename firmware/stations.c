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

#include "stations.h"

volatile bool overcurrent_detected = false;

station_state_t stations_states[NUMBER_OF_STATIONS];

void stations_init(void)
{
	station_state_t* station = &stations_states[0];
	for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n, ++station)
	{
		station->run_time = 0;
		station->open = false;
	}

	// main valve is always closed
	PORTBbits.RB4 = CLOSE_VALVE;

	// close station valves 1-8
	stations_close_all();

	// setup valve ports to output
	TRISA &= 0b11011111;
	TRISB &= 0b11001111;
	TRISC &= 0b00000011;

	// enable PWM signal to generate offset to comparator C1INA voltage
	PR2 = _XTAL_FREQ / 4 / 5000; // 5kHz frequency
	T2CON = 0b00000100;	// enable, post-scaler 1:1, pre-scaler 1
	CCPTMRS0 = 0;
	ECCP3AS = 0;
	ECCP3DEL = 0;
	PSTR3CON = 0b00000001;
	// enable PWM mode
	CCP3CONbits.CCP3M = 0b1100;
	// configure PWM duty cycle
	CCP3CONbits.DC3B = 0;
	CCPR3L = ECCP3_PWM_DUTY_CYCLE;
	// PWM port as output
	TRISGbits.TRISG0 = 0;

	// enable voltage comparator (C1INB - shunt resistor input, C1INA - shunt resistor output)
	CM1CON = 0b10010000; // enable, compare C1INB < C1INA(+offset), interrupt on high-to-low transition
	TRISF |= 0b01100000; // C1INA, C1INB ports as input
	PIR6bits.CMP1IF = 0;
	PIE6bits.CMP1IE = 1;
}

bool stations_queue_start(uint8_t number, uint16_t run_time)
{
	station_state_t* station = &stations_states[number];

	if (station->run_time >= run_time)
		return false;

	station->run_time = run_time;
	return true;
}

void stations_queue_stop(void)
{
	station_state_t* station = &stations_states[0];
	for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n, ++station)
		station->run_time = 0;
}

bool stations_queue_progress(uint16_t* run_time)
{
	station_state_t* station_state = &stations_states[0];
	for (uint8_t n = 0; n < NUMBER_OF_STATIONS; ++n, ++station_state)
	{
		if (station_state->run_time > 0 || station_state->open)
		{
			*run_time = station_state->run_time;
			return true;
		}
	}

	return false;
}

void stations_queue_update(uint8_t elapsed_seconds)
{
	bool any_valve_in_rush = false;
	uint8_t valves_opened = 0;

	station_state_t* station_state = &stations_states[0];
	for (uint8_t m = 0; m < NUMBER_OF_STATIONS; ++m, ++station_state)
	{
		if (station_state->open)
		{
			// handle opened station valves
			if (station_state->run_time < elapsed_seconds)
			{
				station_state->run_time = 0;
				station_state->open = false;
				stations_close_single(m);
				continue;
			}

			station_state->run_time -= elapsed_seconds;

			++valves_opened;
		}
		else
		{
			// handle not-opened station valves
			if (station_state->run_time > 0)
			{
				if (any_valve_in_rush)
					// don't open valve yet, other one is opening right now (wait to finish its in-rush time)
					continue;

				if (valves_opened >= 2)
					// don't allow to open more than 2 valves in same time (be careful about overall power consumption)
					continue;

				station_state->open = true;
				stations_open_single(m);

				any_valve_in_rush = true;
				++valves_opened;
			}
		}
	}
}

uint8_t stations_queue_mask(void)
{
	uint8_t mask = 0;

	station_state_t* station_state = &stations_states[0];
	for (uint8_t m = 0; m < NUMBER_OF_STATIONS; ++m, ++station_state)
		if (station_state->open)
			mask |= (1 << m);

	return mask;
}

void stations_open_single(uint8_t number)
{
	// do NOT open any valve if short circuit was detected and not yet cleared
	if (overcurrent_detected)
		return;

	switch (number)
	{
		case 0:
			PORTBbits.RB5 = OPEN_VALVE;
			break;
		case 1:
			PORTCbits.RC5 = OPEN_VALVE;
			break;
		case 2:
			PORTCbits.RC4 = OPEN_VALVE;
			break;
		case 3:
			PORTCbits.RC3 = OPEN_VALVE;
			break;
		case 4:
			PORTCbits.RC2 = OPEN_VALVE;
			break;
		case 5:
			PORTCbits.RC7 = OPEN_VALVE;
			break;
		case 6:
			PORTCbits.RC6 = OPEN_VALVE;
			break;
		case 7:
			PORTAbits.RA5 = OPEN_VALVE;
			break;
	}
}

void stations_close_single(uint8_t number)
{
	switch (number)
	{
		case 0:
			PORTBbits.RB5 = CLOSE_VALVE;
			break;
		case 1:
			PORTCbits.RC5 = CLOSE_VALVE;
			break;
		case 2:
			PORTCbits.RC4 = CLOSE_VALVE;
			break;
		case 3:
			PORTCbits.RC3 = CLOSE_VALVE;
			break;
		case 4:
			PORTCbits.RC2 = CLOSE_VALVE;
			break;
		case 5:
			PORTCbits.RC7 = CLOSE_VALVE;
			break;
		case 6:
			PORTCbits.RC6 = CLOSE_VALVE;
			break;
		case 7:
			PORTAbits.RA5 = CLOSE_VALVE;
			break;
	}
}

void stations_close_all(void)
{
	PORTBbits.RB5 = CLOSE_VALVE;
	PORTCbits.RC5 = CLOSE_VALVE;
	PORTCbits.RC4 = CLOSE_VALVE;
	PORTCbits.RC3 = CLOSE_VALVE;
	PORTCbits.RC2 = CLOSE_VALVE;
	PORTCbits.RC7 = CLOSE_VALVE;
	PORTCbits.RC6 = CLOSE_VALVE;
	PORTAbits.RA5 = CLOSE_VALVE;
}

void stations_overcurrent_detected(void)
{
//	if (CMSTATbits.CMP1OUT)
//		// should never happen
//		return;

	// overcurrent detected!
	PORTBbits.RB4 = CLOSE_VALVE;
	PORTBbits.RB5 = CLOSE_VALVE;
	PORTCbits.RC5 = CLOSE_VALVE;
	PORTCbits.RC4 = CLOSE_VALVE;
	PORTCbits.RC3 = CLOSE_VALVE;
	PORTCbits.RC2 = CLOSE_VALVE;
	PORTCbits.RC7 = CLOSE_VALVE;
	PORTCbits.RC6 = CLOSE_VALVE;
	PORTAbits.RA5 = CLOSE_VALVE;

	overcurrent_detected = true;
}