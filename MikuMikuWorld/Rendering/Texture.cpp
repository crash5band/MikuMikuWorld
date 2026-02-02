#include "../File.h"
#include "../IO.h"
#include "Texture.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
#include "stb_image.h"

using namespace IO;

namespace MikuMikuWorld
{
	Texture::Texture(const std::string& filename) :
		Texture(filename, TextureFilterMode::Linear, TextureFilterMode::Linear)
	{
	}

	Texture::Texture(const std::string& filename, TextureFilterMode min, TextureFilterMode mag) :
		glID{ 0 }, width{ 0 }, height{ 0 }, filename{ filename }
	{
		name = File::getFilenameWithoutExtension(filename);
		if (!File::exists(filename))
			return;

		read(filename, min, mag);

		std::string sprSheet = File::getFilepath(filename) + "spr/" + name + ".txt";
		if (File::exists(sprSheet))
		{
			readSprites(sprSheet);
		}
		else
		{
			sprites.push_back(Sprite(0, 0, width, height));
		}
	}

	Texture::Texture(const std::string& filename, TextureFilterMode filter) :
		Texture(filename, filter, filter)
	{

	}

	void Texture::bind() const
	{
		glBindTexture(GL_TEXTURE_2D, glID);
	}

	void Texture::dispose()
	{
		if (glID > 0)
		{
			glDeleteTextures(1, &glID);
			width = height = glID = 0;
		}
	}

	void Texture::readSprites(const std::string& filename)
	{
		File f(filename, FileMode::Read);
		std::vector<std::string> lines = f.readAllLines();
		f.close();

		for (auto& line : lines)
		{
			line = trim(line);
			if (!isComment(line, "#") && !line.empty())
				sprites.push_back(parseSprite(line));
		}
	}

	Sprite Texture::parseSprite(const std::string& line)
	{
		std::vector<std::string> values = split(line, ",");
		int x = atoi(values[0].c_str());
		int y = atoi(values[1].c_str());
		int w = atoi(values[2].c_str());
		int h = atoi(values[3].c_str());

		return Sprite(x, y, w, h);
	}

	void Texture::read(const std::string& filename, TextureFilterMode minFilter, TextureFilterMode magFilter)
	{
		glGenTextures(1, &glID);
		glBindTexture(GL_TEXTURE_2D, glID);

		int nrChannels;
		stbi_set_flip_vertically_on_load(0);
		auto data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 4);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (GLint)minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (GLint)magFilter);

		glBindTexture(GL_TEXTURE_2D, 0);
		free(data);
	}
}