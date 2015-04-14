/*
    Copyright (c) 2015 Andrey Nakin
    All Rights Reserved

	This file is part of v7-28-arduino project.

	v7-28-arduino is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	v7-28-arduino is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with v7-28-arduino.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CONSTS_H_V7_28_ARDUINO
#define	__CONSTS_H_V7_28_ARDUINO

#ifndef V7_28_REVISION_NO
#define V7_28_REVISION_NO "1"
#endif

#ifndef V7_28_REVISION_DATE
#define V7_28_REVISION_DATE __DATE__
#endif

#define V7_28_IDN ( "V7-28," V7_28_REVISION_NO "," V7_28_REVISION_DATE )

/* DPD pins 7, Ж, Е */
#define V7_28_CODE_SIGN_NON_POLAR 0
#define V7_28_CODE_SIGN_OVERFLOW 2
#define V7_28_CODE_SIGN_POSITIVE 3
#define V7_28_CODE_SIGN_NEGATIVE 5

/* DPD pins К, 11, 12, Л */
#define V7_28_READ_MODE_DCV				0x05
#define V7_28_READ_MODE_ACV				0x05
#define V7_28_READ_MODE_RESISTANCE_4W	0x02
#define V7_28_READ_MODE_DCV_RATIO		0x0f
#define V7_28_READ_MODE_ACV_RATIO		0x0f

#define V7_28_SET_MODE_DELAY		500
#define V7_28_SET_MODE_MAX_STEPS	4

/* RC pins А, 2, Б, 3, В */
#define V7_28_MODE_DCV				0x0e
#define V7_28_MODE_ACV				0x06
#define V7_28_MODE_RESISTANCE_4W	0x09
#define V7_28_MODE_DCV_RATIO		0x1f
#define V7_28_MODE_ACV_RATIO		0x17

#define V7_28_RANGE_DCV_0	0.1
#define V7_28_RANGE_DCV_1	1.0
#define V7_28_RANGE_DCV_2	10.0
#define V7_28_RANGE_DCV_3	100.0
#define V7_28_RANGE_DCV_4	1000.0

#define V7_28_RANGE_ACV_0	1.0
#define V7_28_RANGE_ACV_1	10.0
#define V7_28_RANGE_ACV_2	100.0
#define V7_28_RANGE_ACV_3	1000.0

#define V7_28_RANGE_RESISTANCE_0	0.1e3
#define V7_28_RANGE_RESISTANCE_1	1.0e3
#define V7_28_RANGE_RESISTANCE_2	10.0e3
#define V7_28_RANGE_RESISTANCE_3	100.0e3
#define V7_28_RANGE_RESISTANCE_4	1000.0e3
#define V7_28_RANGE_RESISTANCE_5	10000.0e3

/* RC pins 5, Д, 6, Е */
#define V7_28_RANGE_CODE_DCV_0	0xc
#define V7_28_RANGE_CODE_DCV_1	0x0
#define V7_28_RANGE_CODE_DCV_2	0x8
#define V7_28_RANGE_CODE_DCV_3	0x2
#define V7_28_RANGE_CODE_DCV_4	0xa

#define V7_28_RANGE_CODE_ACV_0	0x0
#define V7_28_RANGE_CODE_ACV_1	0x8
#define V7_28_RANGE_CODE_ACV_2	0x2
#define V7_28_RANGE_CODE_ACV_3	0xa

#define V7_28_RANGE_CODE_RESISTANCE_0	0xc
#define V7_28_RANGE_CODE_RESISTANCE_1	0x0
#define V7_28_RANGE_CODE_RESISTANCE_2	0x8
#define V7_28_RANGE_CODE_RESISTANCE_3	0x2
#define V7_28_RANGE_CODE_RESISTANCE_4	0xa
#define V7_28_RANGE_CODE_RESISTANCE_5	0x1

#define V7_28_STATE_INITIALIZED_MODE	0x01

#define	V7_28_DEF_OVERRUN	1.2
#define V7_28_RESOLUTION 1.0e-5
#define	V7_28_NPLC 100.0
#define	V7_28_AUTO_ZERO TRUE
#define	V7_28_AUTO_ZERO_ONCE FALSE
#define V7_28_INPUT_IMPEDANCE_AUTO TRUE

#define	V7_28_DEFAULT_BAUD_RATE	9600

/* non-standard error codes */
#define V7_28_ERROR_SET_MODE	(SCPI_ERROR_INTERNAL_START + 1)
#define V7_28_ERROR_MODE_NOT_INITIALIZED	(SCPI_ERROR_INTERNAL_START + 2)
#define V7_28_ERROR_RANGE_NOT_INITIALIZED	(SCPI_ERROR_INTERNAL_START + 3)
#define V7_28_ERROR_RESOLUTION_NOT_INITIALIZED	(SCPI_ERROR_INTERNAL_START + 4)
#define	V7_28_ERROR_AUTO_RANGE_NOT_INITIALIZED	(SCPI_ERROR_INTERNAL_START + 5)

#endif	//	__CONSTS_H_V7_28_ARDUINO

