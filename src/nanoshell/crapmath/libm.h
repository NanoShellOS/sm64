#ifndef _LIBM_H
#define _LIBM_H

#include <stdint.h>

#define WANT_ROUNDING 1
#define issignaling_inline(x) 0
#  define INFINITY (__builtin_inff ())

typedef double double_t;


static uint64_t asuint64(double dbl)
{
	return *((uint64_t*)&dbl);
}

static double asdouble(uint64_t dbl)
{
	return *((double*)&dbl);
}

static inline double eval_as_double(double x)
{
	double y = x;
	return y;
}

double fabs(double x)
/*{
	if (x < 0)
		return -x;
	return x;
}*/
;

static inline void fp_force_eval(double x)
{
	volatile __attribute__((unused)) double y;
	y = x;
}

static inline double fp_barrier(double x)
{
	volatile double y = x;
	return y;
}
static double __math_divzero(uint32_t sign)
{
    return fp_barrier(sign ? -1.0 : 1.0) / 0.0;
}

static double __math_xflow(uint32_t sign, double y)
{
	return eval_as_double(fp_barrier(sign ? -y : y) * y);
}
static double __math_uflow(uint32_t sign)
{
	return __math_xflow(sign, 0x1p-767);
}

static double __math_oflow(uint32_t sign)
{
	return __math_xflow(sign, 0x1p769);
}
static double __math_invalid(double x)
{
	return (x - x) / (x - x);
}

#define predict_false(x) __builtin_expect(x, 0)

#endif
