#include <Arduino.h>
#include <scpimm/scpimm.h>
#include "config.h"
#include "consts.h"
#include "pins.h"

/******************************************************************************
  Forward declarations
******************************************************************************/

static scpimm_mode_t supported_modes();
static int16_t set_mode(const scpimm_mode_t mode, const scpimm_mode_params_t* params);
static int16_t get_mode(scpimm_mode_t* mode, scpimm_mode_params_t* params);
static int16_t get_possible_range(scpimm_mode_t mode, double* min_range, double* max_range);
static int16_t get_possible_resolution(scpimm_mode_t  mode, double range, double* min_resolution, double* max_resolution);
static bool_t start_measure();
static size_t send(const uint8_t* data, size_t len);
static void set_remote(bool_t remote, bool_t lock);
static const char* get_error_description(int16_t error);
static void set_auto_range(const bool_t enabled);

/******************************************************************************
  Global constants
******************************************************************************/

static scpimm_interface_t scpimm_interface = {
	supported_modes,
	set_mode,
	get_mode,
	get_possible_range,
	get_possible_resolution,
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
	double range;
	bool_t auto_range;
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
    if (digitalRead(PIN_MAINTENANCE) || !digitalRead(PIN_MEAS_START)) {
        return;  // no valid data
    }
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

    attachInterrupt(INT_MEAS_START, valueIsReady, RISING);
    
	set_disabled(TRUE);
	set_auto_range(TRUE);	//  enable autorange

    digitalWrite(PIN_REMOTE, LOW);  //  enable remote mode
    digitalWrite(PIN_AUTOSTART, LOW);   //  disable autostart
    digitalWrite(PIN_START, LOW);
    
	set_disabled(FALSE);
}

static void set_auto_range(const bool_t enabled) {
	digitalWrite(PIN_AUTO_RANGE, enabled ? HIGH : LOW);
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

static uint8_t get_dcv_range_code(double range) {
	if (range <= 1.0e-1) {
		return V7_28_RANGE_DCV_0;
	}
	if (range <= 1.0e0) {
		return V7_28_RANGE_DCV_1;
	}
	if (range <= 1.0e1) {
		return V7_28_RANGE_DCV_2;
	}
	if (range <= 1.0e2) {
		return V7_28_RANGE_DCV_3;
	}
	return V7_28_RANGE_DCV_4;
}

static uint8_t get_acv_range_code(double range) {
	if (range <= 1.0e0) {
		return V7_28_RANGE_ACV_0;
	}
	if (range <= 1.0e1) {
		return V7_28_RANGE_ACV_1;
	}
	if (range <= 1.0e2) {
		return V7_28_RANGE_ACV_2;
	}
	return V7_28_RANGE_ACV_3;
}

static uint8_t get_resistance_range_code(double range) {
	if (range <= 1.0e-1) {
		return V7_28_RANGE_RESISTANCE_0;
	}
	if (range <= 1.0e0) {
		return V7_28_RANGE_RESISTANCE_1;
	}
	if (range <= 1.0e1) {
		return V7_28_RANGE_RESISTANCE_2;
	}
	if (range <= 1.0e2) {
		return V7_28_RANGE_RESISTANCE_3;
	}
	if (range <= 1.0e3) {
		return V7_28_RANGE_RESISTANCE_4;
	}
	return V7_28_RANGE_RESISTANCE_5;
}

static void set_range(const scpimm_mode_t mode, double range) {
	uint8_t code;

	range *= 0.99;

	switch (mode) {
	case SCPIMM_MODE_DCV:
	case SCPIMM_MODE_DCV_RATIO:
		code = get_dcv_range_code(range);
		break;

	case SCPIMM_MODE_ACV:
		code = get_acv_range_code(range);
		break;

	case SCPIMM_MODE_RESISTANCE_2W:
		code = get_resistance_range_code(range);
		break;

	default:
		return;
	}

	write_4bits(PIN_RANGE_1, PIN_RANGE_2, PIN_RANGE_4, PIN_RANGE_8, code);
}

/******************************************************************************
  Multimeter interface implementation
******************************************************************************/

static scpimm_mode_t supported_modes() {
	return SCPIMM_MODE_DCV | SCPIMM_MODE_DCV_RATIO | SCPIMM_MODE_ACV
		| SCPIMM_MODE_RESISTANCE_2W;
}

static int16_t set_mode(const scpimm_mode_t mode, const scpimm_mode_params_t* params) {
	bool_t change_mode = TRUE;
	uint8_t mode_code, expected;
	int n = V7_28_SET_MODE_MAX_STEPS;

	if (is_mode_initialized()) {
		change_mode = mode != v7_28_state.mode;
	}

	if (change_mode) {
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
	}

	// disable remote control before we change state of control pins
    set_disabled(TRUE);

    if (change_mode) {
    	write_5bits(PIN_MODE_1, PIN_MODE_2, PIN_MODE_4, PIN_MODE_8, PIN_MODE_16, mode_code);
    }

	if (params) {
		set_range(mode, params->range);
		v7_28_state.range = params->range;
		set_state_initialized(V7_28_STATE_INITIALIZED_RANGE);

		set_auto_range(params->auto_range);
		v7_28_state.auto_range = params->auto_range;
		set_state_initialized(V7_28_STATE_INITIALIZED_AUTO_RANGE);

		// params->resolution is ignored
	}

	// enable remote control for changes to take an effect
    set_disabled(FALSE);

    if (change_mode) {
    	// wait until multimeter actually switches to given mode
		while (n--) {
			const uint8_t read_mode = read_4bits(PIN_READ_MODE_1, PIN_READ_MODE_2, PIN_READ_MODE_4, PIN_READ_MODE_8);
			if (read_mode == expected) {
				v7_28_state.mode = mode;
				set_state_initialized(V7_28_STATE_INITIALIZED_MODE);
				return SCPI_ERROR_OK;
			}

			delay(V7_28_SET_MODE_DELAY);
		}

		// multimeter could not switch to given mode
		return V7_28_ERROR_SET_MODE;
    } else {
    	// no need to wait
    	return SCPI_ERROR_OK;
    }
}

static int16_t get_mode(scpimm_mode_t* mode, scpimm_mode_params_t* params) {
	if (mode) {
		if (!is_mode_initialized()) {
			return V7_28_ERROR_MODE_NOT_INITIALIZED;
		}
		*mode = v7_28_state.mode;
	}

	if (params) {
		if (!is_state_initialized(V7_28_STATE_INITIALIZED_AUTO_RANGE)) {
			return V7_28_ERROR_AUTO_RANGE_NOT_INITIALIZED;
		}
		params->auto_range = v7_28_state.auto_range;

		// TODO
	}

	return SCPI_ERROR_OK;
}

static int16_t get_possible_range(const scpimm_mode_t mode, double* const min_range, double* const max_range) {
	double min_value, max_value;

	switch (mode) {
	case SCPIMM_MODE_DCV:
	case SCPIMM_MODE_DCV_RATIO:
		min_value = 0.1;
		max_value = 1000.0;
		break;

	case SCPIMM_MODE_ACV:
		min_value = 1.0;
		max_value = 1000.0;
		break;

	case SCPIMM_MODE_RESISTANCE_2W:
		min_value = 0.1;
		max_value = 10000.0;
		break;

	default:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	if (min_range) {
		*min_range = min_value;
	}

	if (max_range) {
		*max_range = max_value;
	}

	return SCPI_ERROR_OK;
}

static int16_t get_possible_resolution(const scpimm_mode_t /* mode */, const double range, double* const min_resolution, double* const max_resolution) {

	/*
	 * always return the same resolution which is 1.0e-6 of given range
	 * actual resolution is 10 times worse, we're just trying emulate HP 34401A
	 */
	const double resolution = range * 1.0e-6;

	if (min_resolution) {
		*min_resolution = resolution;
	}

	if (max_resolution) {
		*max_resolution = resolution;
	}

	return SCPI_ERROR_OK;
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

	SCPIMM_setup(&scpimm_interface);
}

void loop() {
	if (Serial.available() > 0) {
		const char p = Serial.read();
		SCPIMM_parseInBuffer(&p, 1);
	}
}

