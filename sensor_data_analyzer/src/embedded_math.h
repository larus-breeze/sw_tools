/***********************************************************************//**
 * @file    		embedded_math.h
 * @brief   		defines platform-dependent algorithms and constants
 * @author		Dr. Klaus Schaefer
 * @copyright 		Copyright 2021 Dr. Klaus Schaefer. All rights reserved.
 * @license 		This project is released under the GNU Public License GPL-3.0

    <Larus Flight Sensor Firmware>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 **************************************************************************/

#ifndef INC_EMBEDDED_MATH_H_
#define INC_EMBEDDED_MATH_H_

#ifndef _WIN32
char* itoa( int value, char* result, int base=10);
#endif

#ifdef _MSC_VER
# include "corecrt_math_defines.h"
#endif
#ifdef _WIN32
# include <cmath>  // needed for inline abs
#endif

#include "math.h"
#include "stdint.h"
#include "float.h"
#include "assert.h"

#define ftype float

#define ZERO 0.0f
#define ONE 1.0f
#define TWO 2.0f
#define HALF 0.5f
#define QUARTER 0.25f
#define M_PI_F M_PI

#define SQR(x) ((x)*(x))
#define SQRT(x) sqrtf(x)
#define COS(x) cosf(x)
#define SIN(x) sinf(x)
#define ASIN(x) asinf(x)
#define ATAN2(y, x) atan2f(y, x)

template <typename type> type CLIP( type x, type min, type max)
{
  return x < min ? min : x > max ? max : x;
}

#endif /* INC_EMBEDDED_MATH_H_ */
