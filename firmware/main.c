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

#include "config.h"
#include "ui.h"
#include "controls.h"
#include "programs.h"
#include "stations.h"
#include "rtcc.h"
#include "sensor.h"
#include "remote.h"
#include "types.h"

volatile bool ac_sensed = false;
uint8_t ac_sense_timeout = 0;

volatile bool rain_sensed = false;

volatile uint8_t ticks_seconds = 0;
volatile uint8_t ticks_fractions = 0;

void sleep(void);

void __interrupt() isr(void)
{
	if (PIR1bits.TMR1IF)
	{
		// last tick of minute period will be handled in RTCC interrupt to sync it with RTCC alarm
		if (ticks_seconds < 59 || (ticks_fractions & 7) < 7)
		{
			TMR1H = 0xFE;
			TMR1L = 0x00;

			if ((++ticks_fractions & 7) == 0)
				ticks_seconds++;
		}

		PIR1bits.TMR1IF = 0;
	}

	if (PIR3bits.RTCCIF)
	{
		// every whole minute reset the fractions timer and count from zero again
		TMR1H = 0xFE;
		TMR1L = 0x00;

		ticks_seconds = 0;
		ticks_fractions = 0;

		PIR3bits.RTCCIF = 0;
	}

	if (PIR6bits.CMP1IF)
	{
		stations_overcurrent_detected();
		PIR6bits.CMP1IF = 0;
	}
}

void main(void)
{
	// setup all ports to digital
	ANCON0 = 0;
	ANCON1 = 0;
	ANCON2 = 0;

	RCON = 0b00111111;
	WDTCON = 0b10010001;

	// sleep control (transistor Q10)
	TRISJbits.TRISJ0 = 0;

	rtcc_init();
	sensor_init();
	remote_init();
	stations_init();
	programs_init();
	ui_init();

	// INT0 on falling edge to sense AC
	INTCON2bits.INTEDG0 = 0;

	// enable interrupts
	INTCONbits.GIE = 1;
	INTCONbits.PEIE = 1;

	// enable RTCC alarm every minute
	rtcc_enable_alarm();
	rtcc_sync();

	// sync timer to RTCC second tick
	while (!RTCCFGbits.HALFSEC);
	while (RTCCFGbits.HALFSEC);
	PIE3bits.RTCCIE = 1;

	// configure timer1 to 1:8 pre-scaler, SOSC clock source, 125ms ticks
	T1CON = 0b10111111;
	T1GCON = 0;
	TMR1H = 0xFE;
	TMR1L = 0x00;
	PIR1bits.TMR1IF = 0;
	PIE1bits.TMR1IE = 1;

	uint8_t last_seconds = 0;
	uint8_t last_fraction = 0;
	for (;;)
	{
		CLRWDT();

		// restart AC sense
		INTCONbits.INT0IF = 0;

		// wait for another 125ms period elapsed
		while (last_fraction == ticks_fractions);
		last_fraction = ticks_fractions;

		// check if at least one whole second elapsed
		if (last_seconds != ticks_seconds)
		{
			uint8_t elapsed_seconds = ((ticks_seconds >= last_seconds) ? ticks_seconds : (ticks_seconds + 60)) - last_seconds;
			last_seconds = ticks_seconds;

			if (ticks_seconds == 0)
			{
				// sync current time
				rtcc_sync();

				if (ui_selection() == FUNCTION_RUN)
				{
					if (!rain_sensed)
						// check program and eventually schedule stations to run
						programs_check(&now);
				}
			}

			if (ticks_seconds == 15 || ticks_seconds == 45)
			{
				// check the sensor every 30 seconds
				bool rain_was_sensed = rain_sensed;
				rain_sensed = sensor_check();
				if (rain_sensed && !rain_was_sensed)
				{
					stations_queue_stop();

					// sensor has just begun to detect rain, reset the program calendar offsets
					programs_reset_calendar(&now);
				}
			}

			stations_queue_update(elapsed_seconds);
		}

		ui_update();

		// if any falling edge was detected on INT0, AC is sensed
		ac_sensed = INTCONbits.INT0IF;
		if (ac_sensed)
			ac_sense_timeout = 0;
		else if (++ac_sense_timeout == 40)
			// no AC was sensed in 5 seconds, go to sleep and save the battery
			sleep();

		// enable remote input, sensor bypass and current sense only if AC is sensed
		PORTJbits.RJ0 = ac_sensed;

		// handle remote requests
		remote_handle();
	}

	return;
}

void sleep(void)
{
#ifndef RELEASE
	// sleep enabled only in release build
	return;
#endif
	INTCON = 0;

	// switch to internal oscillator, but keep RTCC SOSCGO oscillator running
	OSCCON = 0b00110010;
	OSCCON2 = 0b00001010;
	PMD1 = 0b00100000;

	// disable comparator voltage reference
	CVRCON = 0;

	// disable high/low voltage detection
	HLVDCON = 0;

	// disable pulse steering control
	PSTR1CON = 0;
	PSTR2CON = 0;
	PSTR3CON = 0;

	// disable timers
	T0CON = 0;
	T1CON = 0;
	T1GCON = 0;
	T2CON = 0;
	T3CON = 0;
	T3GCON = 0;

	// disable reference oscillator output
	REFOCON = 0;

	// disable peripheral open-drain
	ODCON1 = 0;
	ODCON2 = 0;
	ODCON3 = 0;

	// set comparators +in=CVREF, -in=VBG
	CM1CON = 0b00000111;
	CM2CON = 0b00000111;
	CM3CON = 0b00000111;

	// disable CTMU
	CTMUCONH = 0;
	CTMUICON = 0;

	// disable ADC
	ANCON0 = 0;
	ANCON1 = 0;
	ANCON2 = 0;
	ADCON0 = 0;
	ADCON1 = 0;

	// disable LCD
	LCDCON = 0;

	// resistor ladder is at maximum resistance (minimum contrast)
	LCDREF = 0b00111000;
	LCDRL = 0;

	// regulator voltage goes to ultra low-power sleep
	WDTCON = 0b10010000;

	// set ports to default low-power state
	LATA = 0b00101001;
	LATB = 0b00110000;
	LATC = 0b11111100;
	LATD = 0b00000000;
	LATE = 0b00000000;
	LATF = 0b01100000;
	LATG = 0b00101111;
	LATH = 0b00100000;
	LATJ = 0b00000000;

	TRISA = 0b00000000;
	TRISB = 0b11001111;
	TRISC = 0b00000010;
	TRISD = 0b00000000;
	TRISE = 0b10000111;
	TRISF = 0b01100000;
	TRISG = 0b00101100;
	TRISH = 0b00000000;
	TRISJ = 0b00000000;

	// set PGD/PGC latch to hard-wired levels (version selection)
	TRISBbits.TRISB7 = 1;
	LATBbits.LATB7 = PORTBbits.RB7;
	TRISBbits.TRISB7 = 0;
	TRISBbits.TRISB6 = 1;
	LATBbits.LATB6 = PORTBbits.RB6;
	TRISBbits.TRISB6 = 0;

	// PORTE pull-up enable bit
	PADCFG1 = 0b01000000;

	// disable interrupts
	INTCON = 0;
	INTCON2 = 0;
	INTCON3 = 0;
	PIE1 = 0;
	PIE2 = 0;
	PIE3 = 0;
	PIE4 = 0;
	PIE5 = 0;
	PIE6 = 0;

	// enable interrupt on INT0 (wake when AC is sensed)
	INTCONbits.INT0IE = 1;
	// enable interrupt on INT1 (wake when any button is pressed)
	INTCON3bits.INT1IE = 1;

	// go to sleep and eventually wake up to reset
	OSCCONbits.IDLEN = 0;
	NOP();
	NOP();
	SLEEP();
	NOP();
	NOP();
	NOP();
	RESET();
}