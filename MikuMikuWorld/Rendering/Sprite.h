#pragma once
#include <string>

namespace MikuMikuWorld
{
	class Sprite
	{
	private:
		float x, y, width, height;
		std::string texture;

	public:
		Sprite(const std::string& tex, float _x, float _y, float _w, float _h);

		float getX() const;
		float getY() const;
		float getWidth() const;
		float getHeight() const;
	};
}

