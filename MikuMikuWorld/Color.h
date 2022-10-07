#pragma once

namespace MikuMikuWorld
{
	class Color
	{
	public:
		float r, g, b, a;

		Color(float _r = 0.0f, float _g = 0.0f, float _b = 0.0f, float _a = 1.0f)
			: r{ _r }, g{ _g }, b{ _b }, a{ _a }
		{

		}

		static int rgbaToInt(int r, int g, int b, int a);
		static int abgrToInt(int a, int b, int g, int r);
	};
}