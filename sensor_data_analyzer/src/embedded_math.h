/**
 * @file    embedded_math.h
 * @brief   settings for special embedded target
 * @author  Dr. Klaus Schaefer dr.klaus.schaefer@mail.de
 */
#ifndef INC_EMBEDDED_MATH_H_
#define INC_EMBEDDED_MATH_H_

#include "math.h"
#include "stdint.h"
#include "float.h"

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

inline int ROUND(float x) { return (int)((x) + 0.5f);}
#define CLIP( x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

#endif /* INC_EMBEDDED_MATH_H_ */
