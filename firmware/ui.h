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

void ui_init(void);
void ui_update(void);

void ui_run(void);
void ui_current_time(void);
void ui_start_times(void);
void ui_run_times(void);
void ui_calendar(void);
void ui_seasonal_adjustment(void);
void ui_solar_sync(void);
void ui_manual(void);
void ui_off(void);
void ui_start_stations(void);
void ui_progress(void);

uint8_t ui_selection(void);
void ui_change_selection(uint8_t number);