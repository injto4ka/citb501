#ifndef __MATH_H_
#define __MATH_H_
#include "utils.h"

struct Point2f
{
	float x, y;
	Point2f():x(0), y(0){}
	Point2f(float x0, float y0):x(x0), y(y0){}
};

inline Point2f operator +(const Point2f &pt1, const Point2f &pt2) { return Point2f(pt1.x + pt2.x, pt1.y + pt2.y); }
inline Point2f operator -(const Point2f &pt1, const Point2f &pt2) { return Point2f(pt1.x - pt2.x, pt1.y - pt2.y); }
inline float operator *(const Point2f &pt1, const Point2f &pt2)   { return pt1.x * pt2.x + pt1.y * pt2.y; }
inline Point2f operator *(const Point2f &pt, float n) { return Point2f(pt.x * n, pt.y * n); }
inline Point2f operator *(float n, const Point2f &pt) { return Point2f(pt.x * n, pt.y * n); }
inline Point2f operator /(const Point2f &pt, float n) { return Point2f(pt.x / n, pt.y / n); }

bool IntersectSegmentSegment2D(
	float x1a, float y1a, float x2a, float y2a,
	float x1b, float y1b, float x2b, float y2b,
	float *ka = NULL, float *kb = NULL);

bool IntersectSegmentCircle2D(
	float x1, float y1,
	float x2, float y2,
	float xc, float yc,
	float rc,
	float *k = NULL);

#endif __MATH_H_