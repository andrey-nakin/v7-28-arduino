#include <Arduino.h>
#include <scpimm/scpimm.h>
#include "config.h"
#include "consts.h"
#include "pins.h"

/******************************************************************************
  Forward declarations
******************************************************************************/

static scpimm_mode_t supported_modes();
static int16_t set_mode(const scpimm_mode_t mode, const scpi_number_t* range, const scpi_number_t* resolution);
static int16_t get_mode(scpimm_mode_t* mode, scpi_number_t* range, scpi_number_t* resolution);
static bool_t set_range(scpimm_mode_t mode, const scpi_number_t* range);
static bool_t get_range(scpimm_mode_t mode, scpi_number_t* range);
static bool_t start_measure();
static size_t send(const uint8_t* data, size_t len);
static void set_remote(bool_t remote, bool_t lock);
static const char* get_error_description(int16_t error);

/******************************************************************************
  Global constants
******************************************************************************/

static scpimm_interface_t scpimm_interface = {
	supported_modes,
	set_mode,
	get_mode,
	start_measure,
	send,
	set_remote,
	NULL,
	NULL,
	NULL,
	get_error_description
};

/******************************************************************************
  Global variables
******************************************************************************/

static struct {
	uint8_t initialized;
	scpimm_mode_t mode;
	scpi_number_t range;
	scpi_number_t resolution;
} v7_28_state;

/******************************************************************************
  Internal functions
******************************************************************************/

static uint8_t read_1bit(int pin1) {
	return (uint8_t) (digitalRead(pin1) ? 1 : 0);
}

static uint8_t read_3bits(int pin1, int pin2, int pin4) {
	return (uint8_t) (
		(digitalRead(pin4) ? 4 : 0)
		| (digitalRead(pin2) ? 2 : 0)
		| (digitalRead(pin1) ? 1 : 0)
	);
}

static uint8_t read_4bits(int pin1, int pin2, int pin4, int pin8) {
	return (uint8_t) (
		(digitalRead(pin8) ? 8 : 0)
		| (digitalRead(pin4) ? 4 : 0)
		| (digitalRead(pin2) ? 2 : 0)
		| (digitalRead(pin1) ? 1 : 0)
	);
}

static void write_4bits(int pin1, int pin2, int pin4, int pin8, uint8_t value) {
	digitalWrite(pin1, (value & 0x01) != 0 ? HIGH : LOW);
	digitalWrite(pin2, (value & 0x02) != 0 ? HIGH : LOW);
	digitalWrite(pin4, (value & 0x04) != 0 ? HIGH : LOW);
	digitalWrite(pin8, (value & 0x08) != 0 ? HIGH : LOW);
}

static void write_5bits(int pin1, int pin2, int pin4, int pin8, int pin16, uint8_t value) {
	digitalWrite(pin1, (value & 0x01) != 0 ? HIGH : LOW);
	digitalWrite(pin2, (value & 0x02) != 0 ? HIGH : LOW);
	digitalWrite(pin4, (value & 0x04) != 0 ? HIGH : LOW);
	digitalWrite(pin8, (value & 0x08) != 0 ? HIGH : LOW);
	digitalWrite(pin16, (value & 0x10) != 0 ? HIGH : LOW);
}

static void readNumber(scpi_number_t * result) {
	const uint8_t d6 = read_1bit(PIN_DIGIT6_1);
	const uint8_t d5 = read_4bits(PIN_DIGIT5_1, PIN_DIGIT5_2, PIN_DIGIT5_4, PIN_DIGIT5_8);
	const uint8_t d4 = read_4bits(PIN_DIGIT4_1, PIN_DIGIT4_2, PIN_DIGIT4_4, PIN_DIGIT4_8);
	const uint8_t d3 = read_4bits(PIN_DIGIT3_1, PIN_DIGIT3_2, PIN_DIGIT3_4, PIN_DIGIT3_8);
	const uint8_t d2 = read_4bits(PIN_DIGIT2_1, PIN_DIGIT2_2, PIN_DIGIT2_4, PIN_DIGIT2_8);
	const uint8_t d1 = read_4bits(PIN_DIGIT1_1, PIN_DIGIT1_2, PIN_DIGIT1_4, PIN_DIGIT1_8);
	const uint8_t sign = read_3bits(PIN_SIGN_1, PIN_SIGN_2, PIN_SIGN_4);

	if (V7_28_CODE_SIGN_OVERFLOW == sign) {
    	result->type = SCPI_NUM_NUMBER;    // TODO
		return;
	}

	result->type = SCPI_NUM_NUMBER;

	double res = 
		  1.0e-5 * d1
		+ 1.0e-4 * d2
		+ 1.0e-3 * d3
		+ 1.0e-2 * d4
		+ 1.0e-1 * d5
		+ 1.0 * d6;

	if (V7_28_CODE_SIGN_NEGATIVE == sign) {
		res = -res;
	}

	uint8_t order = read_3bits(PIN_ORDER_1, PIN_ORDER_2, PIN_ORDER_4);
	if (digitalRead(PIN_ORDER_SIGN)) {
		while (order--) {
			res *= 0.1;
		}
	} else {
		while (order--) {
			res *= 10.0;
		}
	}

	result->value = res;
}

static void valueIsReady() {
	scpi_number_t num;
	readNumber(&num);
	SCPIMM_read_value(&num);
}

static void set_disabled(bool_t disabled) {
    digitalWrite(PIN_DISABLE, disabled ? LOW : HIGH);
}

static void setupPins() {
	Serial.begin(9600);
  
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

	pinMode(PIN_READ_MODE_1, INPUT);
	pinMode(PIN_READ_MODE_2, INPUT);
	pinMode(PIN_READ_MODE_4, INPUT);
	pinMode(PIN_READ_MODE_8, INPUT);

	pinMode(PIN_MEAS_START, INPUT);
	pinMode(PIN_CPU_START, INPUT);
	pinMode(PIN_MAINTENANCE, INPUT);

	pinMode(PIN_RANGE_1, OUTPUT);
	pinMode(PIN_RANGE_2, OUTPUT);
	pinMode(PIN_RANGE_4, OUTPUT);
	pinMode(PIN_RANGE_8, OUTPUT);

	pinMode(PIN_MODE_1, OUTPUT);
	pinMode(PIN_MODE_2, OUTPUT);
	pinMode(PIN_MODE_4, OUTPUT);
	pinMode(PIN_MODE_8, OUTPUT);
	pinMode(PIN_MODE_16, OUTPUT);

	pinMode(PIN_START, OUTPUT);
	pinMode(PIN_REMOTE, OUTPUT);
	pinMode(PIN_AUTOSTART, OUTPUT);
	pinMode(PIN_DISABLE, OUTPUT);

	pinMode(PIN_RESERVED_5V_1, OUTPUT);

    attachInterrupt(5, valueIsReady, RISING);
    
	set_disabled(TRUE);

    digitalWrite(PIN_RESERVED_5V_1, HIGH);
    digitalWrite(PIN_REMOTE, LOW);
    digitalWrite(PIN_AUTOSTART, LOW);
    digitalWrite(PIN_AUTO_RANGE, HIGH);
    digitalWrite(PIN_START, HIGH);
    
	set_disabled(FALSE);
}

static void setRange(const uint8_t bits) {
    set_disabled(TRUE);
	write_4bits(PIN_RANGE_1,PIN_RANGE_2, PIN_RANGE_4, PIN_RANGE_8, bits); 
	digitalWrite(PIN_AUTO_RANGE, LOW);
    set_disabled(FALSE);
}

static void setAutoRange() {
	digitalWrite(PIN_AUTO_RANGE, HIGH);
}

static uint8_t min_range_code(scpimm_mode_t mode) {
	switch (mode) {
		case SCPIMM_MODE_DCV:
		case SCPIMM_MODE_DCV_RATIO:
		case SCPIMM_MODE_RESISTANCE_2W:
			return 0x0c;

		case SCPIMM_MODE_ACV:
			return 0x00;
	}

	return 0;
}

static uint8_t max_range_code(scpimm_mode_t mode) {
	switch (mode) {
		case SCPIMM_MODE_DCV:
		case SCPIMM_MODE_DCV_RATIO:
		case SCPIMM_MODE_ACV:
			return 0x0a;			

		case SCPIMM_MODE_RESISTANCE_2W:
			return 0x01;
			break;			
	}

	return 0;
}

static void set_number_range(float r) {
	setRange(
		r <= 0.11f ? 0xc0
		: r <= 1.1f ? 0x00
		: r <= 11.0f ? 0x80
		: r <= 110.0f ? 0x02
		: r <= 1100.0f ? 0x0a
		: 0x01
	);
}

static bool_t is_state_initialized(uint8_t bits) {
	return bits == (v7_28_state.initialized & bits);
}

static void set_state_initialized(uint8_t bits) {
	v7_28_state.initialized |= bits;
}

static bool_t is_mode_initialized(void) {
	return is_state_initialized(V7_28_STATE_INITIALIZED_MODE);
}

static int16_t set_mode_impl(const scpimm_mode_t mode, const scpi_number_t* range, const scpi_number_t* resolution) {
	uint8_t mode_code, expected;
	int n = V7_28_SET_MODE_MAX_STEPS;

	switch (mode) {
		case SCPIMM_MODE_DCV:
			mode_code = V7_28_MODE_DCV;
			expected = V7_28_READ_MODE_DCV;
			break;			

		case SCPIMM_MODE_DCV_RATIO:
			mode_code = V7_28_MODE_DCV_RATIO;
			expected = V7_28_READ_MODE_DCV_RATIO;
			break;			

		case SCPIMM_MODE_ACV:
			mode_code = V7_28_MODE_ACV;
			expected = V7_28_READ_MODE_ACV;
			break;			

		case SCPIMM_MODE_RESISTANCE_2W:
			mode_code = V7_28_MODE_RESISTANCE_2W;
			expected = V7_28_READ_MODE_RESISTANCE_2W;
			break;			

		default:
			// mode is not supported
			return V7_28_ERROR_INVALID_MODE;
	}

    set_disabled(TRUE);
	write_5bits(PIN_MODE_1, PIN_MODE_2, PIN_MODE_4, PIN_MODE_8, PIN_MODE_16, mode_code);
    set_disabled(FALSE);

	while (n--) {
		const uint8_t read_mode = read_4bits(PIN_READ_MODE_1, PIN_READ_MODE_2, PIN_READ_MODE_4, PIN_READ_MODE_8);
		if (read_mode == expected) {
			v7_28_state.mode = mode;
			set_state_initialized(V7_28_STATE_INITIALIZED_MODE);
			return SCPI_ERROR_OK;
		}

		delay(V7_28_SET_MODE_DELAY);
	}

	// multimeter could not set to given mode
	return V7_28_ERROR_SET_MODE;
}

/******************************************************************************
  Multimeter interface implementation
******************************************************************************/

static scpimm_mode_t supported_modes() {
	return SCPIMM_MODE_DCV | SCPIMM_MODE_DCV_RATIO | SCPIMM_MODE_ACV 
		| SCPIMM_MODE_RESISTANCE_2W;
}

static int16_t set_mode(const scpimm_mode_t mode, const scpi_number_t* range, const scpi_number_t* resolution) {
	if (!is_mode_initialized() || v7_28_state.mode != mode) {
		// current mode is different or not specified, change it to given one
		return set_mode_impl(mode, range, resolution);
	} else {
		// keep current mode intact

		// TODO check range, resolution
		return SCPI_ERROR_OK;
	}
}

static int16_t get_mode(scpimm_mode_t* mode, scpi_number_t* range, scpi_number_t* resolution) {
	if (mode) {
		if (!is_mode_initialized()) {
			return V7_28_ERROR_MODE_NOT_INITIALIZED;
		}
		*mode = v7_28_state.mode;
	}

	if (range) {
		if (!is_state_initialized(V7_28_STATE_INITIALIZED_RANGE)) {
			return V7_28_ERROR_RANGE_NOT_INITIALIZED;
		}
		*range = v7_28_state.range;
	}

	if (resolution) {
		if (!is_state_initialized(V7_28_STATE_INITIALIZED_RESOLUTION)) {
			return V7_28_ERROR_RESOLUTION_NOT_INITIALIZED;
		}
		*resolution = v7_28_state.resolution;
	}

	return SCPI_ERROR_OK;
}

/* TODO deprecated */
static bool_t set_range(scpimm_mode_t mode, const scpi_number_t* range) {
	switch (range->type) {
		case SCPI_NUM_MIN:
			setRange(min_range_code(mode));
			break;

		case SCPI_NUM_MAX:
			setRange(max_range_code(mode));
			break;

		case SCPI_NUM_DEF:
			setAutoRange();
			break;

		case SCPI_NUM_NUMBER:
			set_number_range((float) range->value);
			break;

		default:
			/* invalid parameters */
			return FALSE;
	}

	return TRUE;
}

/* TODO deprecated */
static bool_t get_range(scpimm_mode_t mode, scpi_number_t* range) {
	switch (range->type) {
		case SCPI_NUM_MIN:
			switch (mode) {
				case SCPIMM_MODE_DCV:
				case SCPIMM_MODE_DCV_RATIO:
				case SCPIMM_MODE_RESISTANCE_2W:
					range->value = 0.1;
					return TRUE;
				case SCPIMM_MODE_ACV:
					range->value = 1.0;
					return TRUE;
			}
			break;

		case SCPI_NUM_MAX:
			switch (mode) {
				case SCPIMM_MODE_DCV:
				case SCPIMM_MODE_ACV:
				case SCPIMM_MODE_DCV_RATIO:
					range->value = 1000.0;
					return TRUE;

				case SCPIMM_MODE_RESISTANCE_2W:
					range->value = 10000.0;
					return TRUE;
			}
			break;
	
		default:
			break;
	}

	/* invalid parameters */
	return FALSE;
}

static bool_t start_measure() {
	digitalWrite(PIN_START, HIGH);
	delayMicroseconds(20);
	digitalWrite(PIN_START, LOW);
	return TRUE;
}

static size_t send(const uint8_t* data, const size_t len) {
	return Serial.write(data, len);
}

static void set_remote(bool_t remote, bool_t lock) {
	(void) lock;
	digitalWrite(PIN_REMOTE, remote ? LOW : HIGH);
}

static const char* get_error_description(int16_t error) {
	switch (error) {
		case V7_28_ERROR_INVALID_MODE:
			return "Invalid mode";
		case V7_28_ERROR_SET_MODE:
			return "Cannot set mode";
	}

	return NULL;
}

/******************************************************************************
  Arduino entry point
******************************************************************************/

void setup() {
	Serial.begin(BAUD_RATE);
	setupPins();
	set_mode(SCPIMM_MODE_DCV, NULL, NULL);

	SCPIMM_setup(&scpimm_interface);
}

void loop() {
	if (Serial.available() > 0) {
		const char p = Serial.read();
		SCPIMM_parseInBuffer(&p, 1);
	}
}

