/*
 Copyright (C) 2012 Sebastien Jean <baz dot jean at gmail dot com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the version 3 GNU General Public License as
 published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <Arduino.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <FSK600BaudTA900TB1500Mod.h>
#include <pins.h>

#define LOG_FILE_PATH "data.txt"

#define SENSOR_DATA_ASCII_STRING_LENGTH 40

#define KIWI_FRAME_LENGTH 11

FSK600BaudTA900TB1500Mod fskModulator(FSK_MODULATOR_TX);

char sensorDataAsAsciiString[SENSOR_DATA_ASCII_STRING_LENGTH];

File logFile;

int absolutePressureSensorValue = 0;

int differentialPressureSensorValue = 0;

int internalTemperatureSensorValue = 0;

int externalTemperatureSensorValue = 0;

int batteryVoltageSensorValue = 0;

// Kiwi (Planete-Sciences/ CNES format) Frame
unsigned char kiwiFrame[KIWI_FRAME_LENGTH];

/**
 * Application's main (what else to say?)
 * @return (never)
 */
int main(void) {
	init();

	setup();

	for (;;)
		loop();

	return 0;
}

/**
 * Initializes LEDs wirings
 */
void initLEDs()
{
	pinMode(RED_LED, OUTPUT);
	pinMode(ORANGE_LED, OUTPUT);
	pinMode(GREEN_LED, OUTPUT);
	pinMode(BLUE_LED, OUTPUT);
}

/**
 * Initializes SD shield
 */
// TODO fix comment
int initSdShield()
{
	pinMode(SD_CARD_CHIP_SELECT, OUTPUT);
	return SD.begin(SD_CARD_CHIP_SELECT);
}


/**
 * Initializes User switch
 */
void initUserButton()
{
	pinMode(USER_BUTTON, INPUT);
	digitalWrite(USER_BUTTON, HIGH);
}

/**
 * Initializes serial debug communication
 */
void initDebug()
{
	Serial.begin(600);
}


/**
 * Resets KIWI frame contents
 */
void resetKiwiFrame()
{
	for (int i = 0; i < 11; i++)
		kiwiFrame[i] = 0x00;
	kiwiFrame[0] = 0xFF;
}

/**
 * Plays LEDs startup sequence
 */
void showLEDsStartupSequence()
{
	digitalWrite(RED_LED, HIGH);
	digitalWrite(ORANGE_LED, HIGH);
	digitalWrite(GREEN_LED, HIGH);
	digitalWrite(BLUE_LED, HIGH);
	delay(1000);
	digitalWrite(RED_LED, LOW);
	digitalWrite(ORANGE_LED, LOW);
	digitalWrite(GREEN_LED, LOW);
	digitalWrite(BLUE_LED, LOW);
}

/**
 * Displays status (OK/KO) using red/green LEDs
 */
void showStatus(int status)
{
	if (status)
	{
		digitalWrite(GREEN_LED,HIGH);
		digitalWrite(RED_LED,LOW);
	}
	else
	{
		digitalWrite(GREEN_LED,LOW);
		digitalWrite(RED_LED,HIGH);
	}
}

void quicklyMakeSomeLedBlinkSeveralTimes(int led, int times)
{
	for (int i=0;i<times;i++)
	{
		digitalWrite(led, HIGH);
		delay(100);
		digitalWrite(led, LOW);
		delay(100);
	}
}

int logMessageOnSdCard(char *message)
{
	logFile = SD.open(LOG_FILE_PATH, FILE_WRITE);
	if (logFile)
	{
		logFile.println(message);
		logFile.close();
	}
	return logFile;
}

/**
 * Arduino's setup function, called once at startup, after init
 */
void setup()
{
	initLEDs();

	showLEDsStartupSequence();

	initUserButton();

	initDebug();

	// SD card init
	Serial.print(F("SD Init..."));

	if (!initSdShield())
	{
		Serial.println(F("KO"));
		showStatus(0);
	}
	else
	{
		Serial.println(F("OK"));
		showStatus(0);
		delay(1000);
		if (digitalRead(USER_BUTTON) == 0)
		{
			// delete the file:
			Serial.println(F("SD_C"));
			SD.remove(LOG_FILE_PATH);
			quicklyMakeSomeLedBlinkSeveralTimes(ORANGE_LED, 5);
		}
		Serial.println(F("R"));
		if (logMessageOnSdCard("R"))
			quicklyMakeSomeLedBlinkSeveralTimes(GREEN_LED, 5);
		else
			quicklyMakeSomeLedBlinkSeveralTimes(RED_LED, 5);
	}

	resetKiwiFrame();

	// wdt_enable(WDTO_8S);
}

/**
 * Arduino's loop function, called in loop (incredible, isn't it ?)
 */
void loop() {
	int sensorStringOffset = 0;
	unsigned char chk = 0;

	// millis since last reset processing
	itoa(millis() / 1000, sensorDataAsAsciiString, 10);
	sensorStringOffset = strlen(sensorDataAsAsciiString);
	sensorDataAsAsciiString[sensorStringOffset++] = ',';

	// absolute pressure processing
	absolutePressureSensorValue = analogRead(ABSOLUTE_PRESSURE_ANALOG_SENSOR);
	itoa(absolutePressureSensorValue, sensorDataAsAsciiString + sensorStringOffset, 10);
	sensorStringOffset = strlen(sensorDataAsAsciiString);
	sensorDataAsAsciiString[sensorStringOffset++] = ',';
	kiwiFrame[1] = (unsigned char) (absolutePressureSensorValue / 4);
	if (kiwiFrame[1] == 0xFF)
		kiwiFrame[1] = 0xFE;

	// differential pressure processing
	differentialPressureSensorValue = analogRead(DIFFERENTIAL_PRESSURE_ANALOG_SENSOR);
	itoa(differentialPressureSensorValue, sensorDataAsAsciiString + sensorStringOffset, 10);
	sensorStringOffset = strlen(sensorDataAsAsciiString);
	sensorDataAsAsciiString[sensorStringOffset++] = ',';
	kiwiFrame[2] = (unsigned char) (differentialPressureSensorValue / 4);
	if (kiwiFrame[2] == 0xFF)
		kiwiFrame[2] = 0xFE;

	// internal temperature pressure processing
	internalTemperatureSensorValue = analogRead(INTERNAL_TEMPERATURE_ANALOG_SENSOR);
	itoa(internalTemperatureSensorValue, sensorDataAsAsciiString + sensorStringOffset, 10);
	sensorStringOffset = strlen(sensorDataAsAsciiString);
	sensorDataAsAsciiString[sensorStringOffset++] = ',';
	kiwiFrame[3] = (unsigned char) (internalTemperatureSensorValue / 4);
	if (kiwiFrame[3] == 0xFF)
		kiwiFrame[3] = 0xFE;

	// external temperature pressure processing
	externalTemperatureSensorValue = analogRead(EXTERNAL_TEMPERATURE_ANALOG_SENSOR);
	itoa(externalTemperatureSensorValue, sensorDataAsAsciiString + sensorStringOffset, 10);
	sensorStringOffset = strlen(sensorDataAsAsciiString);
	sensorDataAsAsciiString[sensorStringOffset++] = ',';
	kiwiFrame[4] = (unsigned char) (externalTemperatureSensorValue / 4);
	if (kiwiFrame[4] == 0xFF)
		kiwiFrame[4] = 0xFE;

	// battery voltage processing
	batteryVoltageSensorValue = analogRead(BATTERY_VOLTAGE_ANALOG_SENSOR);
	itoa(batteryVoltageSensorValue, sensorDataAsAsciiString + sensorStringOffset, 10);
	sensorStringOffset = strlen(sensorDataAsAsciiString);
	sensorDataAsAsciiString[sensorStringOffset++] = '\r';
	sensorDataAsAsciiString[sensorStringOffset++] = '\n';
	kiwiFrame[5] = (unsigned char) (batteryVoltageSensorValue / 4);
	if (kiwiFrame[5] == 0xFF)
		kiwiFrame[5] = 0xFE;
	kiwiFrame[9] = (unsigned char) (batteryVoltageSensorValue / 8);

	for (int cpt = 1; cpt < KIWI_FRAME_LENGTH - 1; cpt++)
		chk = (unsigned char) ((chk + kiwiFrame[cpt]) % 256);

	chk = (unsigned char) (chk / 2);
	kiwiFrame[KIWI_FRAME_LENGTH] = chk;

	// Kiwi Frame transmission
	for (int cpt = 0; cpt < KIWI_FRAME_LENGTH; cpt++)
		fskModulator.write(kiwiFrame[cpt]);
	fskModulator.off();

	// Logging
	logFile = SD.open(LOG_FILE_PATH, FILE_WRITE);
	if (logFile) {
		//Serial.println(F("log file access success"));
		digitalWrite(GREEN_LED, HIGH);
		delay(100);
		digitalWrite(GREEN_LED, LOW);
		delay(100);
		digitalWrite(GREEN_LED, HIGH);
		delay(100);
		digitalWrite(GREEN_LED, LOW);
		logFile.print(sensorDataAsAsciiString);
		logFile.close();
	}
	else
	{
		Serial.println(F("log file access failure"));
		digitalWrite(RED_LED, HIGH);
		delay(100);
		digitalWrite(RED_LED, LOW);
		delay(100);
		digitalWrite(RED_LED, HIGH);
		delay(100);
		digitalWrite(RED_LED, LOW);
	}
		//wdt_reset();

		// Sensor data processing

		// Debug
	Serial.print(sensorDataAsAsciiString);

	// Transmission
	for (int cpt = 0; cpt < strlen(sensorDataAsAsciiString); cpt++)
		fskModulator.write(sensorDataAsAsciiString[cpt]);
	fskModulator.off();
}

