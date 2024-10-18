#pragma once
#include <string>

namespace MikuMikuWorld
{
	class Sprite
	{
	private:
		float x, y, width, height;

	public:
		Sprite(float _x, float _y, float _w, float _h) :
			x{ _x }, y{ _y }, width{ _w }, height{ _h }
		{

		}

		inline float getX1() const { return x; }
		inline float getY1() const { return y; }
		inline float getX2() const { return x + width; }
		inline float getY2() const { return y + height; }
		inline float getWidth() const { return width; }
		inline float getHeight() const { return height; }
	};
}

