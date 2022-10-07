#include "Color.h"

namespace MikuMikuWorld
{
	int Color::rgbaToInt(int r, int g, int b, int a)
	{
		return r << 24 | g << 16 | b << 8 | a;
	}

	int Color::abgrToInt(int a, int b, int g, int r)
	{
		return a << 24 | b << 16 | g << 8 | r;
	}
}