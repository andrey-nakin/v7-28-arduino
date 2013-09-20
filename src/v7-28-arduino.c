#include <Arduino.h>
#include <scpimm/scpimm.h>
#include "config.h"
#include "pins.h"

/******************************************************************************
  Internal functions
******************************************************************************/

static void setupPins() {
	pinMode(PIN_DIGIT1_1, INPUT);
	pinMode(PIN_DIGIT1_2, INPUT);
	pinMode(PIN_DIGIT1_4, INPUT);
	pinMode(PIN_DIGIT1_8, INPUT);

	pinMode(PIN_DIGIT2_1, INPUT);
	pinMode(PIN_DIGIT2_2, INPUT);
	pinMode(PIN_DIGIT2_4, INPUT);
	pinMode(PIN_DIGIT2_8, INPUT);

	pinMode(PIN_DIGIT3_1, INPUT);
	pinMode(PIN_DIGIT3_2, INPUT);
	pinMode(PIN_DIGIT3_4, INPUT);
	pinMode(PIN_DIGIT3_8, INPUT);

	pinMode(PIN_DIGIT4_1, INPUT);
	pinMode(PIN_DIGIT4_2, INPUT);
	pinMode(PIN_DIGIT4_4, INPUT);
	pinMode(PIN_DIGIT4_8, INPUT);

	pinMode(PIN_DIGIT5_1, INPUT);
	pinMode(PIN_DIGIT5_2, INPUT);
	pinMode(PIN_DIGIT5_4, INPUT);
	pinMode(PIN_DIGIT5_8, INPUT);

	pinMode(PIN_DIGIT6_1, INPUT);

	pinMode(PIN_SIGN_1, INPUT);
	pinMode(PIN_SIGN_2, INPUT);
	pinMode(PIN_SIGN_4, INPUT);

	pinMode(PIN_ORDER_1, INPUT);
	pinMode(PIN_ORDER_2, INPUT);
	pinMode(PIN_ORDER_4, INPUT);
	pinMode(PIN_ORDER_SIGN, INPUT);

	pinMode(PIN_RANGE_1, OUTPUT);
	pinMode(PIN_RANGE_2, OUTPUT);
	pinMode(PIN_RANGE_4, OUTPUT);
	pinMode(PIN_RANGE_8, OUTPUT);
	pinMode(PIN_AUTO_RANGE, OUTPUT);

	pinMode(PIN_MODE_1, OUTPUT);
	pinMode(PIN_MODE_2, OUTPUT);
	pinMode(PIN_MODE_4, OUTPUT);
	pinMode(PIN_MODE_8, OUTPUT);

	pinMode(PIN_START, OUTPUT);
}

static uint8_t readDigit(uint8_t pin1, uint8_t pin2, uint8_t pin4, uint8_t pin8) {
	return (uint8_t) (
		(digitalRead(pin8) ? 8 : 0)
		| (digitalRead(pin4) ? 4 : 0)
		| (digitalRead(pin2) ? 2 : 0)
		| (digitalRead(pin1) ? 1 : 0)
	);
}

static float readNumber() {
	const uint8_t sign = 
		(digitalRead(PIN_SIGN_4) ? 4 : 0)
		| (digitalRead(PIN_SIGN_2) ? 2 : 0)
		| (digitalRead(PIN_SIGN_1) ? 1 : 0);
	if (CODE_SIGN_OVERFLOW == sign) {
		return 3.4028235E+38;
	}

	float res = 
		readDigit(PIN_DIGIT1_1, PIN_DIGIT1_2, PIN_DIGIT1_4, PIN_DIGIT1_8)
		+ 10.0 * readDigit(PIN_DIGIT2_1, PIN_DIGIT2_2, PIN_DIGIT2_4, PIN_DIGIT2_8) 
		+ 100.0 * readDigit(PIN_DIGIT3_1, PIN_DIGIT3_2, PIN_DIGIT3_4, PIN_DIGIT3_8) 
		+ 1000.0 * readDigit(PIN_DIGIT4_1, PIN_DIGIT4_2, PIN_DIGIT4_4, PIN_DIGIT4_8) 
		+ 10000.0 * readDigit(PIN_DIGIT5_1, PIN_DIGIT5_2, PIN_DIGIT5_4, PIN_DIGIT5_8)
		+ 100000.0 * (digitalRead(PIN_DIGIT6_1) ? 1 : 0);

	if (CODE_SIGN_NEGATIVE == sign) {
		res = -res;
	}

	uint8_t order =
		(digitalRead(PIN_ORDER_4) ? 4 : 0)
		| (digitalRead(PIN_ORDER_2) ? 2 : 0)
		| (digitalRead(PIN_ORDER_1) ? 1 : 0);
	if (digitalRead(PIN_ORDER_SIGN)) {
		while (order--) {
			res *= 0.1;
		}
	} else {
		while (order--) {
			res *= 10.0;
		}
	}

	return res;
}

static void valueIsReady() {
	SCPIMM_acceptValue(readNumber());
}

static void setRange(const uint8_t bits) {
	digitalWrite(PIN_RANGE_1, (bits & 0x01) != 0 ? HIGH : LOW);
	digitalWrite(PIN_RANGE_2, (bits & 0x02) != 0 ? HIGH : LOW);
	digitalWrite(PIN_RANGE_4, (bits & 0x04) != 0 ? HIGH : LOW);
	digitalWrite(PIN_RANGE_8, (bits & 0x08) != 0 ? HIGH : LOW);
	digitalWrite(PIN_AUTO_RANGE, LOW);
}

static void setAutoRange() {
	digitalWrite(PIN_AUTO_RANGE, HIGH);
}

/******************************************************************************
 Multimeter implementation methods 
******************************************************************************/

int SCPIMM_setMode(const uint8_t mode) {
	switch (mode) {
		case SCPIMM_MODE_DCV:
			digitalWrite(PIN_MODE_1, LOW);
			digitalWrite(PIN_MODE_2, HIGH);
			digitalWrite(PIN_MODE_4, HIGH);
			digitalWrite(PIN_MODE_8, HIGH);
			break;			

		case SCPIMM_MODE_ACV:
			digitalWrite(PIN_MODE_1, LOW);
			digitalWrite(PIN_MODE_2, HIGH);
			digitalWrite(PIN_MODE_4, HIGH);
			digitalWrite(PIN_MODE_8, LOW);
			break;			

		case SCPIMM_MODE_RESISTANCE_2W:
			digitalWrite(PIN_MODE_1, HIGH);
			digitalWrite(PIN_MODE_2, LOW);
			digitalWrite(PIN_MODE_4, LOW);
			digitalWrite(PIN_MODE_8, HIGH);
			break;			

		default:
			// mode is not supported
			return -1;
	}

	return 0;
}

void SCPIMM_setDCVRange(const float max) {
	if (max < 0.0) {
		setRange(
			max <= 0.1 ? 0xc0
			: max <= 1.0 ? 0x00
			: max <= 10.0 ? 0x80
			: max <= 100.0 ? 0x02
			: 0x0a
		);
	} else {
		setAutoRange();
	}
}

void SCPIMM_setACVRange(const float max) {
	if (max < 0.0) {
		setRange(
			max <= 1.0 ? 0x00
			: max <= 10.0 ? 0x80
			: max <= 100.0 ? 0x02
			: 0x0a
		);
	} else {
		setAutoRange();
	}
}

void SCPIMM_setDCCRange(const float max) {
	/* Not supported */
}

void SCPIMM_setACCRange(const float max) {
	/* Not supported */
}

void SCPIMM_setResistanceRange(const float max) {
	if (max < 0.0) {
		setRange(
			max <= 0.1 ? 0xc0
			: max <= 1.0 ? 0x00
			: max <= 10.0 ? 0x80
			: max <= 100.0 ? 0x02
			: max <= 1000.0 ? 0x0a
			: 0x01
		);
	} else {
		setAutoRange();
	}
}

void SCPIMM_triggerMeasurement() {
	// TODO
	digitalWrite(PIN_START, HIGH);
}

size_t SCPIMM_send(const uint8_t* data, const size_t len) {
	return Serial.write(data, len);
}

/******************************************************************************
  Arduino entry point
******************************************************************************/

void setup() {
	Serial.begin(BAUD_RATE);
	setupPins();
	attachInterrupt(INT_VALUE, valueIsReady, FALLING);
}

void loop() {
	if (Serial.available() > 0) {
		const char p = Serial.read();
		SCPIMM_parseInBuffer(&p, 1);
	}
}

