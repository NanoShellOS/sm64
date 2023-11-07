#include <stdint.h>
#include <math.h>

// useful double functions
double sin(double x)
{
    double result;
    __asm__ volatile("fsin" : "=t"(result) : "0"(x));
    return result;
}
double sqrt(double x)
{
    double result;
    __asm__ volatile("fsqrt" : "=t"(result) : "0"(x));
    return result;
}
double cos(double x)
{
    double result;
    __asm__ volatile("fcos" : "=t"(result) : "0"(x));
    return result;
}
double floor(double f)
{
	int x = (int)f;
	if ((double)x > f)
		x--;
	
	return (double) x;
}
double ceil(double f)
{
	int x = (int)f;
	
	if ((double)x < x)
		x++;
	
	return (double) x;
}

float sqrtf(float f)
{
	return (float)sqrt((double)f);
}
float sinf(float f)
{
	return (float)sin((double)f);
}
float cosf(float f)
{
	return (float)cos((double)f);
}
float floorf(float f)
{
	return (float)floor((double)f);
}
float ceilf(float f)
{
	return (float)ceil((double)f);
}
float fabsf(float f)
{
	//quick
	union {
		int a;
		float b;
	} x;
	x.b = f;
	x.a &= ~0x80000000;
	return x.b;
}

long double atan2l(long double x, long double y);
double atan2(double y, double x)
{
	return (double)atan2l((long double)y, (long double)x);
}
