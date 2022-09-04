#pragma once
#include <string>
#include <vector>
#include "Sprite.h"
#include "../File.h"

namespace MikuMikuWorld
{
	enum class WrapMode
	{
		Clamp,
		Repeat
	};

	class Texture
	{
	private:
		std::string name;
		int width;
		int height;
		unsigned int glID;

		Sprite parseSprite(const File &f, const std::string& line);

	public:
		std::vector<Sprite> sprites;

		Texture(const std::string& filename);
		Texture();

		inline int getWidth() const { return width; }
		inline int getHeight() const { return height; }
		inline unsigned int getID() const { return glID; }
		inline const std::string& getName() const { return name; }

		void bind() const;
		void dispose();
		void read(const std::string& filename);
		void readSprites(const std::string& filename);
	};
}