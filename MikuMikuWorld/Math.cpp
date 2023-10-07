#include "Math.h"

namespace MikuMikuWorld
{
	float lerp(float start, float end, float percentage)
	{
		return start + percentage * (end - start);
	}

	float easeIn(float start, float end, float ratio)
	{
		return lerp(start, end, ratio * ratio);
	}

	float easeOut(float start, float end, float ratio)
	{
		return lerp(start, end, 1 - (1 - ratio) * (1 - ratio));
	}

	float midpoint(float x1, float x2)
	{
		return (x1 + x2) * 0.5f;
	}

	bool isWithinRange(float x, float left, float right)
	{
		return x >= left && x <= right;
	}

	std::function<float(float, float, float)> getEaseFunction(EaseType ease)
	{
		switch (ease)
		{
		case EaseType::EaseIn:
			return easeIn;
		case EaseType::EaseOut:
			return easeOut;
		default:
			break;
		}

		return lerp;
	}
}