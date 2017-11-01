#pragma once

#include <math.h>

class Vector2
{
    public:
		Vector2();
		Vector2(float xx, float yy);
		Vector2& operator += (const Vector2& v );
		float Length();
		Vector2 Normalize();

		float x;
		float y;
};

inline Vector2::Vector2() : x(0), y(0)
{    
}

inline Vector2::Vector2(float xx, float yy) 
{
	x = xx;
    y = yy;
}

inline Vector2&	Vector2::operator += (const Vector2& v)
{
	x += v.x;
	y += v.y;
	return *this;
}		

inline Vector2 operator * (const Vector2& v, const float s)
{
	return Vector2(v.x * s, v.y * s);
}

inline float Vector2::Length()
{
	return sqrtf(x*x + y*y);
}

inline Vector2 Vector2::Normalize()
{
    Vector2 t;
    float len = Length();
    t.x = x / len;
    t.y = y / len;
    return t;
}