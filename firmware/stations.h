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

#define NUMBER_OF_STATIONS 8

#if XCORE_VERSION == 2
#define OPEN_VALVE				0
// 59.5%
// 2.927V(A) / 2.60V(B) ~ 0.32V
#define ECCP3_PWM_DUTY_CYCLE	119
#elif XCORE_VERSION == 3
#define OPEN_VALVE				1
// 64%
// 2.05V(A) / 1.73V(B) ~ 0.32V
#define ECCP3_PWM_DUTY_CYCLE	128
#endif
#define CLOSE_VALVE				!OPEN_VALVE

typedef struct
{
	uint16_t run_time;
	bool open;
} station_state_t;

void stations_init(void);

bool stations_queue_start(uint8_t number, uint16_t run_time);
void stations_queue_stop(void);
bool stations_queue_progress(uint16_t* run_time);
void stations_queue_update(uint8_t elapsed_seconds);
uint8_t stations_queue_mask(void);

void stations_open_single(uint8_t number);
void stations_close_single(uint8_t number);
void stations_close_all(void);

void stations_overcurrent_detected(void);