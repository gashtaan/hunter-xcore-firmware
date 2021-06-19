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

//#define DEBUG_ESP_HTTP_SERVER

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "io.h"
#include "packet.h"
#include "firmware.h"

IPAddress ip(192, 168, 1, 19);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 1, 1);

ESP8266WebServer web_server(80);

void updateUnitTime();

void setup(void)
{
#ifdef ARDUINO_ESP8266_WEMOS_D1MINILITE
	Serial.begin(115200);
#endif
	io_init();

	WiFiClient::setDefaultNoDelay(true);
	WiFiClient::setDefaultSync(true);

	WiFi.persistent(false);
	WiFi.setSleepMode(WIFI_NONE_SLEEP);
	WiFi.mode(WIFI_STA);
	WiFi.config(ip, gateway, subnet, dns);
	WiFi.hostname("XCore-Remote");
	WiFi.setAutoReconnect(true);
	WiFi.begin("[ssid]", "[password]");
	while (WiFi.status() != WL_CONNECTED)
		delay(1000);

	updateUnitTime();

	ArduinoOTA.begin();

	web_server.on("/", web_mainPage);
	web_server.on("/startProgram", web_startProgram);
	web_server.on("/startStations", web_startStations);
	web_server.on("/stopStations", web_stopStations);
	web_server.on("/seasonalAdjustment", web_seasonalAdjustment);
	web_server.on("/updateTime", web_updateTime);
	web_server.on("/uploadFirmware", HTTP_POST, [](){ web_server.send(200); }, web_uploadFirmware);
	web_server.begin();
}

void loop(void)
{
	// send dummy BB packet every 5 seconds just to flash LED a bit
	static unsigned long ms = 0;
	if ((millis() - ms) > 5000)
	{
		ms = millis();

		uint8_t packet[] = { 0xBB };
		packet_send(packet, sizeof(packet));
	}

	ArduinoOTA.handle();

	web_server.handleClient();
}

void web_mainPage()
{
	String html = R"HTML(
		<html>
		<title>Hunter X-Core Remote</title>
		<meta name="viewport" content="width=device-width">
		<body>
			RSSI: __RSSI__ dBm<br>
			<hr>
			Start program:<br>
			<form method="get" action="/startProgram">
				<select name="program">
					<option value="1">1</option>
					<option value="2">2</option>
					<option value="3">3</option>
					<option value="4">4</option>
					<option value="5">5</option>
					<option value="6">6</option>
					<option value="7">7</option>
					<option value="8">8</option>
				</select>
				<input type="submit" value="Start">
			</form>

			Start stations:<br>
			<form method="get" action="/startStations">
				1: <input type="text" name="station_1" size="3">
				2: <input type="text" name="station_2" size="3">
				3: <input type="text" name="station_3" size="3">
				4: <input type="text" name="station_4" size="3"><br>
				5: <input type="text" name="station_5" size="3">
				6: <input type="text" name="station_6" size="3">
				7: <input type="text" name="station_7" size="3">
				8: <input type="text" name="station_8" size="3"><br>
				<input type="submit" value="Start">
			</form>

			<form method="get" action="/stopStations">
				<input type="submit" value="Stop">
			</form>
			<hr>
			Seasonal adjustment:<br>
			<form method="get" action="/seasonalAdjustment">
				<input type="text" name="adjustment" size="3">
				<input type="submit" value="Change">
			</form>
			<hr>
			<form method="get" action="/updateTime">
				<input type="submit" value="Update Time">
			</form>
			<hr>
			Upload Firmware:<br>
			<form method="post" action="/uploadFirmware" enctype="multipart/form-data">
				<input type="file" name="data"><br>
				<input type="submit" value="Upload">
			</form>
		</body>
		</html>
	)HTML";

	html.replace("__RSSI__", String(WiFi.RSSI(), DEC));

	web_server.send(200, "text/html", html);
}

void web_startProgram()
{
	uint8_t packet[] = {
		0xA0,
		uint8_t(web_server.arg("program").toInt())
	};
	packet_send(packet, sizeof(packet));

	web_server.sendHeader("Location", "/");
	web_server.send(303);
}

void web_startStations()
{
	uint8_t packet[] = {
		0xA1,
		uint8_t(web_server.arg("station_1").toInt()),
		uint8_t(web_server.arg("station_2").toInt()),
		uint8_t(web_server.arg("station_3").toInt()),
		uint8_t(web_server.arg("station_4").toInt()),
		uint8_t(web_server.arg("station_5").toInt()),
		uint8_t(web_server.arg("station_6").toInt()),
		uint8_t(web_server.arg("station_7").toInt()),
		uint8_t(web_server.arg("station_8").toInt())
	};
	packet_send(packet, sizeof(packet));

	web_server.sendHeader("Location", "/");
	web_server.send(303);
}

void web_stopStations()
{
	uint8_t packet[] = {
		0xA2
	};
	packet_send(packet, sizeof(packet));

	web_server.sendHeader("Location", "/");
	web_server.send(303);
}

void web_seasonalAdjustment()
{
	uint8_t packet[] = {
		0xA3,
		uint8_t(web_server.arg("adjustment").toInt())
	};
	packet_send(packet, sizeof(packet));

	web_server.sendHeader("Location", "/");
	web_server.send(303);
}

void web_updateTime()
{
	updateUnitTime();

	web_server.sendHeader("Location", "/");
	web_server.send(303);
}

void web_uploadFirmware()
{
	static uint8_t data[16384];
	static size_t data_size;
	static bool data_ok;

	HTTPUpload& upload = web_server.upload();
	switch (upload.status)
	{
		case UPLOAD_FILE_START:
			data_size = 0;
			data_ok = true;
			break;

		case UPLOAD_FILE_WRITE:
			if (data_size + upload.currentSize > sizeof(data))
			{
				// not enough space in data buffer
				data_ok = false;
				break;
			}

			memcpy(data + data_size, upload.buf, upload.currentSize);
			data_size += upload.currentSize;
			break;

		case UPLOAD_FILE_END:
			if (data_ok)
			{
				// signal remote unit to prepare, it's going to be reset
				uint8_t packet[] = { 0xB0 };
				packet_send(packet, sizeof(packet));

				if (firmwareUpload(data, data_size))
				{
					web_server.sendHeader("Location", "/");
					web_server.send(303);
					break;
				}
			}
			// fallthrough
		default:
			web_server.send(500, "text/plain", "Upload error!");
			break;
	}
}

void updateUnitTime()
{
	WiFiUDP ntp_udp;
	NTPClient ntp_client(ntp_udp, "europe.pool.ntp.org", 7200);

	ntp_client.begin();

	if (ntp_client.update())
	{
		time_t ntp_time = ntp_client.getEpochTime();
		tm* gm_time = gmtime(&ntp_time);

		uint8_t packet[] = {
			0xA4,
			uint8_t(gm_time->tm_year % 100),
			uint8_t(gm_time->tm_mon + 1),
			uint8_t(gm_time->tm_mday),
			uint8_t(ntp_client.getHours()),
			uint8_t(ntp_client.getMinutes())
		};
		packet_send(packet, sizeof(packet));
	}

	ntp_client.end();
}
