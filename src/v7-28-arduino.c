#include <Arduino.h>
#include <scpimm/scpimm.h>
#include "config.h"
#include "pins.h"

/******************************************************************************
  Forward declarations
******************************************************************************/

static uint16_t supported_modes();
static bool_t set_mode(uint16_t mode, float range, float resolution);
static void trigger();
static size_t send(const uint8_t* data, size_t len);
static void set_remote(bool_t remote, bool_t lock);

/******************************************************************************
  Global constants
******************************************************************************/

static scpimm_interface_t scpimm_interface = {
	supported_modes,
	set_mode,
	trigger,
	send,
	set_remote
};

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
	pinMode(PIN_MODE_16, OUTPUT);

	pinMode(PIN_START, OUTPUT);
	pinMode(PIN_REMOTE, OUTPUT);
	pinMode(PIN_AUTOSTART, OUTPUT);

	digitalWrite(PIN_AUTOSTART, LOW);
}

static uint8_t readDigit(uint8_t pin1, uint8_t pin2, uint8_t pin4, uint8_t pin8) {
	return (uint8_t) (
		(digitalRead(pin8) ? 8 : 0)
		| (digitalRead(pin4) ? 4 : 0)
		| (digitalRead(pin2) ? 2 : 0)
		| (digitalRead(pin1) ? 1 : 0)
	);
}

static double readNumber() {
	const uint8_t sign = 
		(digitalRead(PIN_SIGN_4) ? 4 : 0)
		| (digitalRead(PIN_SIGN_2) ? 2 : 0)
		| (digitalRead(PIN_SIGN_1) ? 1 : 0);
	if (CODE_SIGN_OVERFLOW == sign) {
		return SCPIMM_OVERFLOW;
	}

	double res = 
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
  Multimeter interface implementation
******************************************************************************/

static uint16_t supported_modes() {
	return SCPIMM_MODE_DCV | SCPIMM_MODE_DCV_RATIO | SCPIMM_MODE_ACV 
		| SCPIMM_MODE_RESISTANCE_2W;
}

static bool_t set_mode(const uint16_t mode, float range, float resolution) {
	uint8_t minRange, maxRange;

	switch (mode) {
		case SCPIMM_MODE_DCV:
			digitalWrite(PIN_MODE_1, LOW);
			digitalWrite(PIN_MODE_2, HIGH);
			digitalWrite(PIN_MODE_4, HIGH);
			digitalWrite(PIN_MODE_8, HIGH);
			digitalWrite(PIN_MODE_16, LOW);

			minRange = 0x0c;
			maxRange = 0x0a;			

			break;			

		case SCPIMM_MODE_DCV_RATIO:
			digitalWrite(PIN_MODE_1, HIGH);
			digitalWrite(PIN_MODE_2, HIGH);
			digitalWrite(PIN_MODE_4, HIGH);
			digitalWrite(PIN_MODE_8, HIGH);
			digitalWrite(PIN_MODE_16, HIGH);

			minRange = 0x0c;
			maxRange = 0x0a;			

			break;			

		case SCPIMM_MODE_ACV:
			digitalWrite(PIN_MODE_1, LOW);
			digitalWrite(PIN_MODE_2, HIGH);
			digitalWrite(PIN_MODE_4, HIGH);
			digitalWrite(PIN_MODE_8, LOW);
			digitalWrite(PIN_MODE_16, LOW);

			minRange = 0x00;
			maxRange = 0x0a;			

			break;			

		case SCPIMM_MODE_RESISTANCE_2W:
			digitalWrite(PIN_MODE_1, HIGH);
			digitalWrite(PIN_MODE_2, LOW);
			digitalWrite(PIN_MODE_4, LOW);
			digitalWrite(PIN_MODE_8, HIGH);
			digitalWrite(PIN_MODE_16, LOW);

			minRange = 0x0c;
			maxRange = 0x01;

			break;			

		default:
			// mode is not supported
			return FALSE;
	}

	if (SCPIMM_RANGE_MIN == range) {
		setRange(minRange);
	} else if (SCPIMM_RANGE_MAX == range) {
		setRange(maxRange);
	} else if (SCPIMM_RANGE_DEF == range) {
		setAutoRange();
	} else {
		setRange(
			range <= 0.1 ? 0xc0
			: range <= 1.0 ? 0x00
			: range <= 10.0 ? 0x80
			: range <= 100.0 ? 0x02
			: range <= 1000.0 ? 0x0a
			: 0x01
		);
	}

	return TRUE;
}

static void trigger() {
	digitalWrite(PIN_START, HIGH);
	delayMicroseconds(20);
	digitalWrite(PIN_START, LOW);
}

static size_t send(const uint8_t* data, const size_t len) {
	return Serial.write(data, len);
}

static void set_remote(bool_t remote, bool_t lock) {
	(void) lock;
	digitalWrite(PIN_REMOTE, remote ? LOW : HIGH);
}

/******************************************************************************
  Arduino entry point
******************************************************************************/

void setup() {
	Serial.begin(BAUD_RATE);
	setupPins();
	attachInterrupt(INT_VALUE, valueIsReady, FALLING);
	SCPIMM_setup(&scpimm_interface);
}

void loop() {
	if (Serial.available() > 0) {
		const char p = Serial.read();
		SCPIMM_parseInBuffer(&p, 1);
	}
}

