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
}