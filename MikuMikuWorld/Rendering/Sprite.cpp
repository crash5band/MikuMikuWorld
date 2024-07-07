#include "Sprite.h"

namespace MikuMikuWorld
{
	Sprite::Sprite(const std::string& tex, float _x, float _y, float _w, float _h)
	    : texture{ tex }, x{ _x }, y{ _y }, width{ _w }, height{ _h }
	{
	}

	float Sprite::getX() const { return x; }

	float Sprite::getY() const { return y; }

	float Sprite::getWidth() const { return width; }

	float Sprite::getHeight() const { return height; }
}