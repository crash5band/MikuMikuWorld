#include "Math.h"

namespace MikuMikuWorld
{
	float lerp(float start, float end, float percentage)
	{
		return start + percentage * (end - start);
	}

	float easeIn(float x)
	{
		return x * x;
	}

	float easeOut(float x)
	{
		return 1 - (1 - x) * (1 - x);
	}

	bool isWithinRange(float x, float left, float right)
	{
		return x >= left && x <= right;
	}

	Rectangle::Rectangle(Vector2 start, Vector2 end) :
		x1{ start.x }, y1{ start.y }, x2{ end.x }, y2{ end.y }
	{
	}

	Rectangle::Rectangle(float x, float y, float w, float h) :
		x1{ x }, y1{ y }, x2{ x + w }, y2{ y + h }
	{
	}

	inline float Rectangle::getWidth() const
	{
		return x2 - x1;
	}

	inline float Rectangle::getHeight() const
	{
		return y2 - y1;
	}

	bool Rectangle::intersects(const Vector2& p) const
	{
		return intersects(p.x, p.y);
	}

	bool Rectangle::intersects(float x, float y) const
	{
		return isWithinRange(x, x1, x2) && isWithinRange(y, y1, y2);
	}
}