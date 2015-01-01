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

#define MIN_HESSIAN_DET 0.001f

static inline float GetSplineEps2(const Point *s)
{
	return (s[3].Dist2(s[0]) + s[0].Dist2(s[1]) + s[1].Dist2(s[2]) + s[2].Dist2(s[3])) / 4000000;
}

float DistSplineSpline2(
	const Point *ptSpline1, const Point *ptSpline2,
	float fPrecision,
	float *k1, float *k2)
{
	float
		eps2 = fPrecision > 0 ? (fPrecision * fPrecision) : min(GetSplineEps2(ptSpline1), GetSplineEps2(ptSpline2)),
		mindist2 = FLT_MAX,
		mint1 = 0.0f, mint2 = 0.0f;
	int iters = 0;
	bool bCollision = false;

	const static float fInitSol[][2] = { // initial solutions
		{ 0.00f, 0.00f }, { 0.00f, 0.33f }, { 0.00f, 0.66f }, { 0.00f, 1.00f },
		{ 0.33f, 0.00f }, { 0.33f, 0.33f }, { 0.33f, 0.66f }, { 0.33f, 1.00f },
		{ 0.66f, 0.00f }, { 0.66f, 0.33f }, { 0.66f, 0.66f }, { 0.66f, 1.00f },
		{ 1.00f, 0.00f }, { 1.00f, 0.33f }, { 1.00f, 0.66f }, { 1.00f, 1.00f },
	};

	Point c1[4], c2[4];
	SplineCoefs(ptSpline1, c1);
	SplineCoefs(ptSpline2, c2);

	for(int i=0; i < ArrSize(fInitSol); i++)
	{
		float
			t1 = fInitSol[i][0],
			t2 = fInitSol[i][1];
		Point
			P1 = SplinePos(c1, t1),
			V1 = SplineSpeed(c1, t1),
			A1 = SplineAccel(c1, t1),
			P2 = SplinePos(c2, t2),
			V2 = SplineSpeed(c2, t2),
			A2 = SplineAccel(c2, t2),
			dP = P1 - P2;
		iters++;
		float d2 = dP * dP;
		for(;;)
		{
			/* Gauss - Newton (the fastest iterative method, BUT may deviate if the initial solution is too far away from the real solution)

												| V1.V1 + A1.dP      -V1.V2     |* | V1.dP |
			t(k+1) = t(k) - H*(k).S(k) = t(k) - |                               | .|       |
												|    -V1.V2       V2.V2 - A2.dP |  |-V2.dP |                                 
			dP = P1 - P2
			*/
			float
				H12 = -(V1 * V2),
				H11 = V1 * V1 + A1 * dP,
				H22 = V2 * V2 - A2 * dP,
				det = H11 * H22 - H12 * H12;
			if( det < MIN_HESSIAN_DET || H11 < 0 ) // the hessian has to be positive definite so that there is a minimum
				break;
			float
				S1 = V1 * dP,
				S2 = -(V2 * dP),
				fact = 1.0f / det,
				t1i = t1 + fact * ( H12 * S2 - H22 * S1 ),
				t2i = t2 + fact * ( H12 * S1 - H11 * S2 );

			// check if the next step is somewhere outside
			bool bClamped = false;
			if( t1i < 0 ) { t1i = 0; bClamped = true; }
			else if( t1i > 1 ) { t1i = 1; bClamped = true; }
			if( t2i < 0 ) { t2i = 0; bClamped = true; }
			else if( t2i > 1 ) { t2i = 1; bClamped = true; }
			if( bClamped && t1 == t1i && t2 == t2i )
				break;

			Point P1i, P2i;
			P1i = SplinePos(c1, t1i),
			V1 = SplineSpeed(c1, t1i),
			A1 = SplineAccel(c1, t1i),
			P2i = SplinePos(c2, t2i),
			V2 = SplineSpeed(c2, t2i),
			A2 = SplineAccel(c2, t2i),
			iters++;
			dP = P1i - P2i;
			float d2i = dP * dP;
			if( d2i >= d2 ) // wrong convergence, bad initial solution
				break;
			Point
				dP1 = P1 - P1i,
				dP2 = P2 - P2i;
			P1 = P1i;
			P2 = P2i;
			d2 = d2i;
			t1 = t1i;
			t2 = t2i;
			if( d2 <= eps2 || bClamped )
				break;
			float diff2 = dP1 * dP1 + dP2 * dP2;
			if( diff2 <= eps2 ) // no movement
				break;
		}
		if( mindist2 > d2 )
		{
			mindist2 = d2;
			mint1 = t1;
			mint2 = t2;
			if( mindist2 <= eps2 ) // collision
				bCollision = true;
		}
		if( bCollision )
			break;
	}
	ASSERT(iters < 10*ArrSize(fInitSol));
	if( k1 ) *k1 = mint1;
	if( k2 ) *k2 = mint2;
	return mindist2;
}

float DistSplinePoint2(
	const Point *ptSpline, Point ptPos,
	float fPrecision, float *k)
{
	float
		eps2 = fPrecision > 0 ? (fPrecision * fPrecision) : GetSplineEps2(ptSpline),
		mindist2 = FLT_MAX,
		mint = 0.0f;
	int iters = 0;
	bool bCollision = false;
	Point P2 = ptPos, P2i = P2, c[4];
	SplineCoefs(ptSpline, c);
	static float fInitSol[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
	for(int i=0; i < ArrSize(fInitSol); i++)
	{
		float t = fInitSol[i];
		Point 
			P1 = SplinePos(c, t),
			V1 = SplineSpeed(c, t),
			A1 = SplineAccel(c, t),
			dP = P1 - P2;
		iters++;
		float d2 = dP * dP;
		for(;;)
		{
			/* Gauss - Newton (the fastest iterative method, BUT may deviate if the initial solution is too far away from the real solution)
			t(k+1) = t(k) - H*(k).S(k) = t(k) - V1.dP / (V1.V1 + A1.dP)                            
			*/
			float det = V1 * V1 + A1 * dP;
			if( det < MIN_HESSIAN_DET ) // the hessian has to be positive definite so that there is a minimum
				break;
			float ti = t - (V1 * dP) / det;

			// check if the next step is somewhere outside
			bool bClamped = false;
			if( ti < 0 ) { ti = 0; bClamped = true; }
			else if( ti > 1 ) { ti = 1; bClamped = true; }
			if( bClamped && ti == t )
				break;

			Point P1i;
			P1i = SplinePos(c, ti);
			V1 = SplineSpeed(c, ti);
			A1 = SplineAccel(c, ti);
			iters++;
			dP = P1i - P2i;
			float d2i = dP * dP;
			if( d2i >= d2 ) // wrong convergence, bad initial solution
				break;
			Point dP1 = P1 - P1i;
			P1 = P1i;
			d2 = d2i;
			t = ti;
			if( d2 <= eps2 || bClamped )
				break;
			float diff2 = dP1 * dP1;
			if( diff2 <= eps2 ) // no movement
				break;
		}
		if( mindist2 > d2 )
		{
			mindist2 = d2;
			mint = t;
			if( mindist2 <= eps2 ) // collision
				bCollision = true;
		}
		if( bCollision )
			break;
	}
	ASSERT(iters < 10*ArrSize(fInitSol));
	if( k ) *k = mint;
	return mindist2;
}