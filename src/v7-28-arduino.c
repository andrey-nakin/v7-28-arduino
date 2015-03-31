#include <Arduino.h>
#include <scpimm/scpimm.h>
#include "config.h"
#include "consts.h"
#include "pins.h"

#define STATIC_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(COND) ? 1 : -1]
#define P99_PROTECT(...) __VA_ARGS__
#define FOR_4(expr) expr ## 0, expr ## 1, expr ## 2, expr ## 3
#define FOR_5(expr) expr ## 0, expr ## 1, expr ## 2, expr ## 3, expr ## 4
#define FOR_6(expr) expr ## 0, expr ## 1, expr ## 2, expr ## 3, expr ## 4, expr ## 5
#define REPEAT_4(expr) expr, expr, expr, expr
#define REPEAT_6(expr) expr, expr, expr, expr, expr, expr

/******************************************************************************
  Forward declarations
******************************************************************************/

static int16_t setup_voltmeter();
static int16_t set_mode(const scpimm_mode_t mode, const scpimm_mode_params_t* params);
static int16_t get_mode(scpimm_mode_t* mode);
static int16_t get_allowed_resolutions(scpimm_mode_t mode, size_t range_index, const double** resolutions);
static int16_t start_measure();
static size_t send(const uint8_t* data, size_t len);
static int16_t get_milliseconds(uint32_t* tm);
static int16_t set_interrupt_status(scpi_bool_t disabled);
static int16_t get_global_bool_param(const scpimm_bool_param_t param, scpi_bool_t* value);
static int16_t set_global_bool_param(const scpimm_bool_param_t param, const scpi_bool_t value);
static int16_t get_bool_param(scpimm_mode_t mode, const scpimm_bool_param_t param, scpi_bool_t* value);
static int16_t set_bool_param(scpimm_mode_t mode, const scpimm_bool_param_t param, const scpi_bool_t value);
static int16_t get_numeric_param_values(scpimm_mode_t mode, scpimm_numeric_param_t param, const double** values);
static int16_t get_numeric_param(scpimm_mode_t mode, scpimm_numeric_param_t param, size_t* value_index);
static int16_t set_numeric_param(scpimm_mode_t mode, scpimm_numeric_param_t param, size_t value_index);
static int16_t reset();
static int16_t beep();
static const char* get_idn();

/******************************************************************************
  Defitions
******************************************************************************/

typedef struct {
	const double* ranges;
	const double* overruns;
} v7_28_mode_constants_t;

typedef struct {
	size_t range_index;
	scpi_bool_t auto_range;
} v7_28_mode_params_t;

/******************************************************************************
  Global constants
******************************************************************************/

static scpimm_interface_t scpimm_interface = {
	setup_voltmeter,
	set_mode,
	get_mode,
	get_allowed_resolutions,
	start_measure,
	send,
	get_milliseconds,
	set_interrupt_status,
	get_global_bool_param,
	set_global_bool_param,
	get_bool_param,
	set_bool_param,
	get_numeric_param_values,
	get_numeric_param,
	set_numeric_param,
	reset,
	beep,
	NULL,
	NULL,
	get_idn
};

#define	TERMINATOR	-1.0
#define	RES_ENTRY(range) {range * V7_28_RESOLUTION, TERMINATOR}
#define DECL_MODE_CONSTANTS(func, ranges, overruns, resolutions, codes) \
	static const double func ## _ranges[] =   ranges;	\
	static const double func ## _overruns[] = overruns;	\
	static const double func ## _resolutions[][2] = resolutions;	\
	static const uint8_t func ## _range_codes[] = codes;	\
	STATIC_ASSERT(sizeof(func ## _ranges) / sizeof(func ## _ranges[0]) == sizeof(func ## _overruns) / sizeof(func ## _overruns[0]), func ## _ranges_equals_to_overruns);	\
	STATIC_ASSERT(sizeof(func ## _ranges) / sizeof(func ## _ranges[0]) == sizeof(func ## _resolutions) / sizeof(func ## _resolutions[0]) + 1, func ## _ranges_equals_to_resolutions);	\
	STATIC_ASSERT(sizeof(func ## _ranges) / sizeof(func ## _ranges[0]) == sizeof(func ## _range_codes) / sizeof(func ## _range_codes[0]) + 1, func ## _ranges_equals_to_codes);	\
	static v7_28_mode_constants_t func ## _constants = {func ## _ranges, func ## _overruns};

#define MODE_CONSTANTS(mode)	\
		mode == SCPIMM_MODE_DCV ? &dcv_constants	\
		: mode == SCPIMM_MODE_ACV ? &acv_constants	\
		: mode == SCPIMM_MODE_RESISTANCE_2W ? &resistance_constants	\
		: NULL

#define MODE_PARAMS(mode)	\
		mode == SCPIMM_MODE_DCV ? &v7_28_state.dcv_params	\
		: mode == SCPIMM_MODE_ACV ? &v7_28_state.acv_params	\
		: mode == SCPIMM_MODE_RESISTANCE_2W ? &v7_28_state.resistance_params	\
		: NULL

DECL_MODE_CONSTANTS(
		dcv,
		P99_PROTECT({FOR_5(V7_28_RANGE_DCV_), TERMINATOR}),
		P99_PROTECT({REPEAT_4(V7_28_DEF_OVERRUN), 1.001,  TERMINATOR}),
		P99_PROTECT({RES_ENTRY(V7_28_RANGE_DCV_0), RES_ENTRY(V7_28_RANGE_DCV_1), RES_ENTRY(V7_28_RANGE_DCV_2), RES_ENTRY(V7_28_RANGE_DCV_3), RES_ENTRY(V7_28_RANGE_DCV_4)}),
		P99_PROTECT({FOR_5(V7_28_RANGE_CODE_DCV_)})
);

DECL_MODE_CONSTANTS(
		acv,
		P99_PROTECT({FOR_4(V7_28_RANGE_ACV_), TERMINATOR}),
		P99_PROTECT({REPEAT_4(V7_28_DEF_OVERRUN), TERMINATOR}),
		P99_PROTECT({RES_ENTRY(V7_28_RANGE_ACV_0), RES_ENTRY(V7_28_RANGE_ACV_1), RES_ENTRY(V7_28_RANGE_ACV_2), RES_ENTRY(V7_28_RANGE_ACV_3)}),
		P99_PROTECT({FOR_4(V7_28_RANGE_CODE_ACV_)})
);

DECL_MODE_CONSTANTS(
		resistance,
		P99_PROTECT({FOR_6(V7_28_RANGE_RESISTANCE_), TERMINATOR}),
		P99_PROTECT({REPEAT_6(V7_28_DEF_OVERRUN), TERMINATOR}),
		P99_PROTECT({RES_ENTRY(V7_28_RANGE_RESISTANCE_0), RES_ENTRY(V7_28_RANGE_RESISTANCE_1), RES_ENTRY(V7_28_RANGE_RESISTANCE_2), RES_ENTRY(V7_28_RANGE_RESISTANCE_3), RES_ENTRY(V7_28_RANGE_RESISTANCE_4), RES_ENTRY(V7_28_RANGE_RESISTANCE_5)}),
		P99_PROTECT({FOR_6(V7_28_RANGE_CODE_RESISTANCE_)})
);

static const double nplcs[] = {V7_28_NPLC, TERMINATOR};

/******************************************************************************
  Global variables
******************************************************************************/

static struct {
	uint8_t initialized;
	scpimm_mode_t mode;
	scpi_bool_t remote;
	v7_28_mode_params_t dcv_params, acv_params, dcv_ratio_params, resistance_params;
} v7_28_state;

/******************************************************************************
  Internal functions
******************************************************************************/

static uint8_t read_1bit(int pin1) {
	return (uint8_t) (digitalRead(pin1) ? 1 : 0);
}

// read 3-bit number from pins
static uint8_t read_3bits(int pin1, int pin2, int pin4) {
	return (uint8_t) (
		(digitalRead(pin4) ? 4 : 0)
		| (digitalRead(pin2) ? 2 : 0)
		| (digitalRead(pin1) ? 1 : 0)
	);
}
#define READ_3_BITS(var) read_3bits(var ## 1, var ## 2, var ## 4)

// read 4-bit number from pins
static uint8_t read_4bits(int pin1, int pin2, int pin4, int pin8) {
	return (uint8_t) (
		(digitalRead(pin8) ? 8 : 0)
		| (digitalRead(pin4) ? 4 : 0)
		| (digitalRead(pin2) ? 2 : 0)
		| (digitalRead(pin1) ? 1 : 0)
	);
}
#define READ_4_BITS(var) read_4bits(var ## 1, var ## 2, var ## 4, var ## 8)

// write 4-bit number to pins
static void write_4bits(int pin1, int pin2, int pin4, int pin8, uint8_t value) {
	digitalWrite(pin1, (value & 0x01) != 0 ? HIGH : LOW);
	digitalWrite(pin2, (value & 0x02) != 0 ? HIGH : LOW);
	digitalWrite(pin4, (value & 0x04) != 0 ? HIGH : LOW);
	digitalWrite(pin8, (value & 0x08) != 0 ? HIGH : LOW);
}
#define WRITE_4_BITS(var, value) write_4bits(var ## 1, var ## 2, var ## 4, var ## 8, value)

// write 5-bit number to pins
static void write_5bits(int pin1, int pin2, int pin4, int pin8, int pin16, uint8_t value) {
	digitalWrite(pin1, (value & 0x01) != 0 ? HIGH : LOW);
	digitalWrite(pin2, (value & 0x02) != 0 ? HIGH : LOW);
	digitalWrite(pin4, (value & 0x04) != 0 ? HIGH : LOW);
	digitalWrite(pin8, (value & 0x08) != 0 ? HIGH : LOW);
	digitalWrite(pin16, (value & 0x10) != 0 ? HIGH : LOW);
}
#define WRITE_5_BITS(var, value) write_5bits(var ## 1, var ## 2, var ## 4, var ## 8, var ## 16, value)

// read float number from voltmeter
static void readNumber(scpi_number_t * result) {
#define READ_DIGIT(n) READ_4_BITS(PIN_DIGIT ## n ## _);

	const uint8_t d6 = read_1bit(PIN_DIGIT6_1);
	const uint8_t d5 = READ_DIGIT(5);
	const uint8_t d4 = READ_DIGIT(4);
	const uint8_t d3 = READ_DIGIT(3);
	const uint8_t d2 = READ_DIGIT(2);
	const uint8_t d1 = READ_DIGIT(1);
	const uint8_t sign = READ_3_BITS(PIN_SIGN_);

	if (V7_28_CODE_SIGN_OVERFLOW == sign) {
    	result->type = SCPI_NUM_INF;
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

#undef READ_DIGIT
}

// measurement is completed
static void value_is_ready() {
    if (digitalRead(PIN_MAINTENANCE) || !digitalRead(PIN_MEAS_START)) {
        return;  // no valid data
    }
	scpi_number_t num;
	readNumber(&num);
	SCPIMM_read_value(&num);
}

// external trigger
static void external_trigger() {
	SCPIMM_external_trigger();
}

// set "remote control disabled" state
static void set_disabled(scpi_bool_t disabled) {
    digitalWrite(PIN_DISABLE, disabled ? LOW : HIGH);
}

// configure in/out pins
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
	pinMode(PIN_AUTO_RANGE, OUTPUT);
	pinMode(PIN_DISABLE, OUTPUT);

    attachInterrupt(INT_MEAS_START, value_is_ready, RISING);
    attachInterrupt(INT_EXT_TRIGGER, external_trigger, FALLING);
}

// set "auto range" voltmeter state
static void set_auto_range(const scpi_bool_t enabled) {
	digitalWrite(PIN_AUTO_RANGE, enabled ? HIGH : LOW);
}

// check if given state(s) was/were initialized before
static scpi_bool_t is_state_initialized(uint8_t bits) {
	return bits == (v7_28_state.initialized & bits);
}

// mark state(s) as initialized
static void set_state_initialized(uint8_t bits) {
	v7_28_state.initialized |= bits;
}

// check if mode state was initialized before
static scpi_bool_t is_mode_initialized(void) {
	return is_state_initialized(V7_28_STATE_INITIALIZED_MODE);
}

// setup range pins
static int16_t set_range(const scpimm_mode_t mode, size_t range_index) {
#define	RANGE_CASE(func, var) \
	case SCPIMM_MODE_ ## func:	\
	if (sizeof(var ## _range_codes)/sizeof(var ## _range_codes[0]) <= range_index) {	\
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;	\
	}	\
	code = var ## _range_codes[range_index];	\
	break

	uint8_t code;

	switch (mode) {
		RANGE_CASE(DCV, dcv);
		RANGE_CASE(DCV_RATIO, dcv);
		RANGE_CASE(ACV, acv);
		RANGE_CASE(RESISTANCE_2W, resistance);

	default:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	WRITE_4_BITS(PIN_RANGE_, code);
	return SCPI_ERROR_OK;

#undef	RANGE_CASE
}

static void set_remote(scpi_bool_t remote) {
	digitalWrite(PIN_REMOTE, remote ? LOW : HIGH);
}

/******************************************************************************
  Multimeter interface implementation
******************************************************************************/

static int16_t setup_voltmeter() {
	Serial.begin(BAUD_RATE);
	setupPins();

	return SCPI_ERROR_OK;
}

static int16_t set_mode(const scpimm_mode_t mode, const scpimm_mode_params_t* params) {
	scpi_bool_t change_mode = TRUE;
	uint8_t mode_code, expected;
	int n = V7_28_SET_MODE_MAX_STEPS;
	v7_28_mode_params_t* const mode_params = MODE_PARAMS(mode);

	if (!params) {
		return SCPI_ERROR_UNDEFINED_HEADER;
	}

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
				return SCPI_ERROR_UNDEFINED_HEADER;
		}
	}

	// disable remote control before we change state of control pins
    set_disabled(TRUE);

    if (change_mode) {
    	WRITE_5_BITS(PIN_MODE_, mode_code);
    }

	if (params) {
		mode_params->auto_range = params->auto_range;
		mode_params->range_index = params->range_index;
	}

	set_auto_range(params->auto_range);
	set_range(mode, params->range_index);

	// enable remote control for changes to take an effect
    set_disabled(FALSE);

    if (change_mode) {
    	// wait until multimeter actually switches to given mode
		while (n--) {
			const uint8_t read_mode = READ_4_BITS(PIN_READ_MODE_);
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

static int16_t get_mode(scpimm_mode_t* const mode) {
	if (!is_mode_initialized()) {
		return V7_28_ERROR_MODE_NOT_INITIALIZED;
	}

	if (mode) {
		*mode = v7_28_state.mode;
	}

	return SCPI_ERROR_OK;
}

static int16_t get_allowed_resolutions(scpimm_mode_t mode, size_t range_index, const double** resolutions) {
#define	RANGE_CASE(func, var)	\
	case SCPIMM_MODE_ ## func:	\
		if (sizeof(var ## _resolutions) / sizeof(var ## _resolutions[0]) <= range_index) {	\
			return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;	\
		}	\
		resolutions_src = var ## _resolutions[range_index];	\
		break

	const double *resolutions_src;

	switch (mode) {
		RANGE_CASE(DCV, dcv);
		RANGE_CASE(DCV_RATIO, dcv);
		RANGE_CASE(ACV, acv);
		RANGE_CASE(RESISTANCE_2W, resistance);

		default:
			// mode is not supported
			return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	if (resolutions) {
		*resolutions = resolutions_src;
	}

	return SCPI_ERROR_OK;

#undef	RANGE_CASE
}

static int16_t start_measure() {
	digitalWrite(PIN_START, HIGH);
	delayMicroseconds(20);
	digitalWrite(PIN_START, LOW);
	return SCPI_ERROR_OK;
}

static size_t send(const uint8_t* data, const size_t len) {
	return Serial.write(data, len);
}

static int16_t get_milliseconds(uint32_t* const tm) {
	if (tm) {
		*tm = millis();
	}
	return SCPI_ERROR_OK;
}

static int16_t set_interrupt_status(const scpi_bool_t disabled) {
	if (disabled) {
		noInterrupts();
	} else {
		interrupts();
	}
	return SCPI_ERROR_OK;
}

static int16_t get_global_bool_param(const scpimm_bool_param_t param, scpi_bool_t* value) {
	scpi_bool_t res = FALSE;

	switch (param) {
	case SCPIMM_PARAM_REMOTE:
	case SCPIMM_PARAM_LOCK:
		res = v7_28_state.remote;
		break;

	case SCPIMM_PARAM_INPUT_IMPEDANCE_AUTO:
		// constant non-configurable value
		res = V7_28_INPUT_IMPEDANCE_AUTO;
		break;

	case SCPIMM_PARAM_ZERO_AUTO:
		// constant non-configurable value
		res = V7_28_AUTO_ZERO;
		break;

	case SCPIMM_PARAM_ZERO_AUTO_ONCE:
		// constant non-configurable value
		res = V7_28_AUTO_ZERO_ONCE;
		break;

	case SCPIMM_PARAM_RANGE_AUTO:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	if (value) {
		*value = res;
	}

	return SCPI_ERROR_OK;
}

static int16_t set_global_bool_param(const scpimm_bool_param_t param, const scpi_bool_t value) {
	switch (param) {
	case SCPIMM_PARAM_REMOTE:
		set_remote(v7_28_state.remote = value);
		break;

	// these parameters are not configurable
	// but we do not emit error due to compatibility with Agilent 34401A
	case SCPIMM_PARAM_INPUT_IMPEDANCE_AUTO:
	case SCPIMM_PARAM_ZERO_AUTO:
	case SCPIMM_PARAM_ZERO_AUTO_ONCE:
	case SCPIMM_PARAM_LOCK:
		break;

	case SCPIMM_PARAM_RANGE_AUTO:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	return SCPI_ERROR_OK;
}

static int16_t get_bool_param(const scpimm_mode_t mode, const scpimm_bool_param_t param, scpi_bool_t* const value) {
	const v7_28_mode_params_t* const params = MODE_PARAMS(mode);
	scpi_bool_t res = FALSE;

	if (!params) {
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	switch (param) {
	case SCPIMM_PARAM_RANGE_AUTO:
		res = params->auto_range;
		break;

	case SCPIMM_PARAM_ZERO_AUTO:
	case SCPIMM_PARAM_ZERO_AUTO_ONCE:
	case SCPIMM_PARAM_INPUT_IMPEDANCE_AUTO:
	case SCPIMM_PARAM_REMOTE:
	case SCPIMM_PARAM_LOCK:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	if (value) {
		*value = res;
	}

	return SCPI_ERROR_OK;
}

static int16_t set_bool_param(const scpimm_mode_t mode, const scpimm_bool_param_t param, const scpi_bool_t value) {
	v7_28_mode_params_t* const params = MODE_PARAMS(mode);

	if (!params) {
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	switch (param) {
	case SCPIMM_PARAM_RANGE_AUTO:
		params->auto_range = value;
		if (mode == v7_28_state.mode) {
			set_auto_range(value);
		}
		break;

	case SCPIMM_PARAM_ZERO_AUTO:
	case SCPIMM_PARAM_ZERO_AUTO_ONCE:
	case SCPIMM_PARAM_INPUT_IMPEDANCE_AUTO:
	case SCPIMM_PARAM_REMOTE:
	case SCPIMM_PARAM_LOCK:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	return SCPI_ERROR_OK;
}

static int16_t get_numeric_param_values(scpimm_mode_t mode, scpimm_numeric_param_t param, const double** values) {
	const double* v = NULL;
	const v7_28_mode_constants_t* const constants = MODE_CONSTANTS(mode);

	if (!constants) {
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	switch (param) {
	case SCPIMM_PARAM_RANGE:
		v = constants->ranges;
		break;

	case SCPIMM_PARAM_RANGE_OVERRUN:
		v = constants->overruns;
		break;

	case SCPIMM_PARAM_NPLC:
		v = nplcs;	//	same value for all modes
		break;

	case SCPIMM_PARAM_RESOLUTION:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	if (values) {
		*values = v;
	}

	return SCPI_ERROR_OK;
}

static int16_t get_numeric_param(scpimm_mode_t mode, scpimm_numeric_param_t param, size_t* value_index) {
	const v7_28_mode_params_t* const params = MODE_PARAMS(mode);
	size_t res = 0;

	if (!params) {
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	switch (param) {
	case SCPIMM_PARAM_RANGE:
		res = params->range_index;
		break;

	case SCPIMM_PARAM_RESOLUTION:
		res = 0; //	same value for all modes
		break;

	case SCPIMM_PARAM_NPLC:
		res = 0; //	same value for all modes
		break;

	case SCPIMM_PARAM_RANGE_OVERRUN:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	if (value_index) {
		*value_index = res;	//	same value for all modes
	}

	return SCPI_ERROR_OK;
}

static int16_t set_numeric_param(const scpimm_mode_t mode, const scpimm_numeric_param_t param, const size_t value_index) {
	v7_28_mode_params_t* const params = MODE_PARAMS(mode);

	if (!params) {
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	switch (param) {
	case SCPIMM_PARAM_RANGE:
		params->range_index = value_index;
		if (mode == v7_28_state.mode) {
			set_range(mode, value_index);
		}
		break;

	case SCPIMM_PARAM_RESOLUTION:
	case SCPIMM_PARAM_NPLC:
		// do not use value, this parameter is not configurable in V7-28
		break;

	case SCPIMM_PARAM_RANGE_OVERRUN:
		return SCPI_ERROR_ILLEGAL_PARAMETER_VALUE;
	}

	return SCPI_ERROR_OK;
}

static void reset_mode_params(v7_28_mode_params_t* mp) {
	mp->range_index = 0;
	mp->auto_range = TRUE;
}

static int16_t reset() {
	reset_mode_params(&v7_28_state.dcv_params);
	reset_mode_params(&v7_28_state.dcv_ratio_params);
	reset_mode_params(&v7_28_state.acv_params);
	reset_mode_params(&v7_28_state.resistance_params);

	set_disabled(TRUE);
	set_remote(v7_28_state.remote = TRUE);
	set_auto_range(TRUE);	//  enable autorange

    digitalWrite(PIN_REMOTE, LOW);  //  enable remote mode
    digitalWrite(PIN_AUTOSTART, LOW);   //  disable autostart
    digitalWrite(PIN_START, LOW);

	set_disabled(FALSE);

	return SCPI_ERROR_OK;
}

static int16_t beep() {
	tone(PIN_BEEP, 500, 500);	//	500 Hz 500 ms
	return SCPI_ERROR_OK;
}

static const char* get_idn() {
	return V7_28_IDN;
}

/******************************************************************************
  Arduino entry point
******************************************************************************/

void setup() {
	SCPIMM_setup(&scpimm_interface);
}

void loop() {
	if (Serial.available() > 0) {
		// data arrived from serial port
		const char p = Serial.read();
		SCPIMM_parse_in_buffer(&p, 1);
	} else {
		// no data from serial port, run background routines
		SCPIMM_yield();
	}
}
