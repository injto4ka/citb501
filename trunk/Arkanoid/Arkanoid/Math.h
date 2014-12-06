#ifndef __MATH_H_
#define __MATH_H_
#include "utils.h"

struct Point
{
	float x, y, z;
	Point():x(0), y(0), z(0){}
	Point(float x0, float y0, float z0 = 0):x(x0), y(y0), z(z0){}
	float Dot(const Point &pt) const { return x * pt.x + y * pt.y + z * pt.z; }
	Point Cross(const Point &pt) const { return Point(z * pt.x - x * pt.y, y * pt.z - z * pt.y, x * pt.y - y * pt.x); }
	float Len2() const { return x * x + y * y + z * z; }
	float Dist2(const Point &pt) const { float dx = x - pt.x, dy = y - pt.y, dz = z - pt.z; return dx * dx + dy * dy + dz * dz; }
	Point Normalize() const { float l2 = Len2(); ASSERT(l2 != 0); float r = 1 / sqrtf(l2); return Point(x * r, y * r, z * r); }
	float Angle(const Point &pt) const { float l2 = Len2() * pt.Len2(); ASSERT( l2 != 0 ); return (180 / PI) * acosf(Dot(pt) / sqrtf(l2)); }
	Point operator -() const {  return Point(-x, -y, -z); }
};

inline Point operator +(const Point &pt1, const Point &pt2) { return Point(pt1.x + pt2.x, pt1.y + pt2.y, pt1.z + pt2.z); }
inline Point operator -(const Point &pt1, const Point &pt2) { return Point(pt1.x - pt2.x, pt1.y - pt2.y, pt1.z - pt2.z); }
inline float operator *(const Point &pt1, const Point &pt2) { return pt1.Dot(pt2); }
inline Point operator *(const Point &pt, float n) { return Point(pt.x * n, pt.y * n, pt.z * n); }
inline Point operator *(float n, const Point &pt) { return Point(pt.x * n, pt.y * n, pt.z * n); }
inline Point operator /(const Point &pt, float n) { return Point(pt.x / n, pt.y / n, pt.z / n); }

struct Quaternion {
	float x, y, z, w;
	Quaternion(): w(1), x(0), y(0), z(0) {}
	Quaternion(float x, float y, float z, float w = 0): w(w), x(x), y(y), z(z) {}
	Quaternion(const Point &ptAxis, float fDeg, bool bNormalized = false) 
	{
		float a = fDeg * PI / 360, u = sinf(a);
		if( !bNormalized )
		{
			float l2 = ptAxis.Len2();
			ASSERT( l2 != 0 );
			u /= sqrtf(l2);
		}
		w = cosf(a);
		x = ptAxis.x * u;
		y = ptAxis.y * u;
		z = ptAxis.z * u;
	}
	Quaternion Normalize() 
	{
		float l2 = w*w + x*x + y*y + z*z;
		ASSERT( l2 != 0 );
		float u = 1 / sqrtf(l2);
		return Quaternion(x * u, y * u, z * u, w * u);
	}
	Quaternion Inv() const { return Quaternion(-x, -y, -z, w); }
	// compose rotation
	Quaternion operator *(const Quaternion &q) const 
	{
		return Quaternion(
			w * q.x + x * q.w + y * q.z - z * q.y,
			w * q.y - x * q.z + y * q.w + z * q.x,
			w * q.z + x * q.y - y * q.x + z * q.w,
			w * q.w - x * q.x - y * q.y - z * q.z);
	}
	Quaternion& operator *=(const Quaternion &q)  { return (*this = (*this) * q); }
	// rotate a vector
	Point operator *(const Point &pt) const 
	{
		Quaternion q = (*this) * Quaternion(pt.x, pt.y, pt.z) * this->Inv();
		return Point(q.x, q.y, q.z);
	}
	float GetDeg() const 
	{
		ASSERT( w >= -1 && w <= 1 );
		return (360 / PI) * acosf(w);
	}
};

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

float DistSegmentPoint2D2(
	float x1, float y1,
	float x2, float y2,
	float xc, float yc,
	float *k = NULL);

inline void SplineCoefs( const Point *pt, Point *c )
{
	c[0] = pt[0];
	c[1] = (pt[1] - pt[0])*3;
	c[2] = (pt[2] - pt[1]*2 + pt[0])*3;
	c[3] = pt[3] - (pt[2] - pt[1])*3 - pt[0];
}

inline Point SplinePos( const Point *c, float t )
{
	return Point(
		((c[3].x * t + c[2].x) * t + c[1].x) * t + c[0].x,
		((c[3].y * t + c[2].y) * t + c[1].y) * t + c[0].y,
		((c[3].z * t + c[2].z) * t + c[1].z) * t + c[0].z);
}

inline Point SplineDir( const Point *c, float t )
{
	return Point(
		(c[3].x * 1.5f * t + c[2].x) * 2.0f * t + c[1].x,
		(c[3].y * 1.5f * t + c[2].y) * 2.0f * t + c[1].y,
		(c[3].z * 1.5f * t + c[2].z) * 2.0f * t + c[1].z);
}

inline Point SplineAccel( const Point *c, float t )
{
	return Point(
		c[3].x * 6.0f * t + c[2].x * 2.0f,
		c[3].y * 6.0f * t + c[2].y * 2.0f,
		c[3].z * 6.0f * t + c[2].z * 2.0f);
}

inline float SplineLenEst( const Point *pt, float *fMinLen = NULL, float *fMaxLen = NULL)
{
	float fMin = sqrtf(pt[3].Dist2(pt[0]));
	float fMax = sqrtf(pt[0].Dist2(pt[1])) + sqrtf(pt[1].Dist2(pt[2])) + sqrtf(pt[2].Dist2(pt[3]));
	if( fMinLen )
		*fMinLen = fMin;
	if( fMaxLen )
		*fMaxLen = fMax;
	return ( fMin + fMax ) / 2;
}

#endif __MATH_H_