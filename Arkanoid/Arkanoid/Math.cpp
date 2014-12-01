#include "math.h"

bool IntersectSegmentSegment2D(
	float x1a, float y1a, float x2a, float y2a,
	float x1b, float y1b, float x2b, float y2b,
	float *ka, float *kb)
{
	float dxa = x2a - x1a, dya = y2a - y1a, dxb = x2b - x1b, dyb = y2b - y1b;
	float c = dxa * dyb - dya * dxb;
	if( !c )
		return false;
	bool positive = c > 0;
	float dx = x1a - x1b, dy = y1a - y1b;
	float a = dxb * dy - dyb * dx;
	if( positive && (a < 0 || a > c ) || !positive && (a > 0 || a < c) )
		return false;
	float b = dxa * dy - dya * dx;
	if( positive && (b < 0 || b > c ) || !positive && (b > 0 || b < c) )
		return false;
	if( ka || kb )
	{
		float cr = 1 / c;
		if( ka ) *ka = a * cr;
		if( kb ) *kb = b * cr;
	}
	return true;
}

bool IntersectSegmentCircle2D(
	float x1, float y1,
	float x2, float y2,
	float xc, float yc,
	float rc,
	float *k)
{
	float
		u = x2 - x1, v = y2 - y1,
		dx = x1 - xc, dy = y1 - yc,
		a = u * u + v * v, b = u * dx + v * dy, c = dx * dx + dy * dy - rc * rc,
		b2 = b * b, d = b2 - a * c;
	ASSERT(a != 0);
	if( d < 0 )
		return false; // no intersection
	float ab = a + b, ab2 = ab * ab;
	if( -b >= 0 && (-ab < 0 || d >= ab2 ) && d <= b2 )
	{
		if( k ) *k = (-b - sqrtf(d)) / a;
		return true; // root 1 (point 1 outside the circle)
	}
	if( ab >= 0 && (  b < 0 || d >= b2  ) && d <= ab2 )
	{
		if( k ) *k = (-b + sqrtf(d)) / a;
		return true; // root 2 (point 1 inside the circle)
	}
	return false; // intersection outside the segment
}

float DistSegmentPoint2D2(
	float x1, float y1,
	float x2, float y2,
	float xc, float yc,
	float *k)
{
	float
		u = x2 - x1, v = y2 - y1,
		dx = x1 - xc, dy = y1 - yc,
		a = u * u + v * v, b = u * dx + v * dy, c = dx * dx + dy * dy;
	if( !a )
		return -1;
	float t;
	if( -b < 0 )
		t = 0;
	else if( -b > a )
		t = 1;
	else
		t = -b / a;
	if( k ) *k = t;
	return ( a * t + 2 * b ) * t + c;
}