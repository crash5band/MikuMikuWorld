#include "ResourceManager.h"
#include "IO.h"
#include <filesystem>
#include <sstream>

namespace MikuMikuWorld
{
	std::vector<Texture> ResourceManager::textures;
	std::vector<Shader*> ResourceManager::shaders;
	std::vector<Transform> ResourceManager::spriteTransform;

	void ResourceManager::loadTexture(const std::string& filename, TextureFilterMode minFilter, TextureFilterMode magFilter)
	{
		if (!IO::File::exists(filename))
		{
			printf("ERROR: ResourceManager::loadTexture() Could not find texture file %s\n", filename.c_str());
			return;
		}

		for (int i = 0; i < textures.size(); ++i)
		{
			if (textures[i].getFilename() == filename)
				return;
		}

		Texture tex(filename, minFilter, magFilter);
		textures.push_back(tex);
	}

	int ResourceManager::getTexture(const std::string& name)
	{
		for (int i = 0; i < textures.size(); ++i)
			if (textures[i].getName() == name)
				return i;

		return -1;
	}

	int ResourceManager::getTextureByFilename(const std::string& filename)
	{
		for (int i = 0; i < textures.size(); ++i)
			if (textures[i].getFilename() == filename)
				return i;

		return -1;
	}

	void ResourceManager::loadShader(const std::string& filename)
	{
		Shader* s = new Shader(IO::File::getFilenameWithoutExtension(filename), filename);
		shaders.push_back(s);
	}

	int ResourceManager::getShader(const std::string& name)
	{
		for (int i = 0; i < shaders.size(); ++i)
			if (shaders[i]->getName() == name)
				return i;

		return -1;
	}

    void ResourceManager::loadTransform(const std::string &filename)
    {
		int idx = 0;
		float transform[8 * 8];
		IO::File file(filename, IO::FileMode::Read);
		
		std::string txt = file.readAllText();
		if (txt.empty())
			return;
		// Remove comments
		for(size_t pos, end; (pos = txt.find('#')) != std::string::npos; )
		{
			end = txt.find('\n', pos);
			if (end != std::string::npos)
				txt.erase(pos, end - pos + 1);
			else
				txt.erase(pos);
		}
		std::stringstream ss(txt);
		float value;
		while(ss >> value)
		{
			transform[idx++] = value;
			if (idx >= std::size(transform))
			{
				idx = 0;
				spriteTransform.push_back(transform);
			}
		}
		if (!ss.eof())
		{
			ss.clear();
			auto invalid_pos = ss.tellg();
			char invalid = ss.get();
			std::string msg = IO::formatString(
				"ResourceManager::loadTransform.\n"
				"Unexpected characters '%c' at position %ld while reading \"%s\"\n", invalid, (long)invalid_pos, filename.c_str());
			throw std::runtime_error(msg);
		}
		if (idx != 0)
		{
			throw std::runtime_error(
				"ResourceManager::loadTransform.\n"
				"Incompleted transform declaration!\n"
			);
		}
    }

	void ResourceManager::disposeTexture(int texID)
	{
		for (int i = 0; i < textures.size(); ++i)
		{
			if (textures[i].getID() == texID)
			{
				textures[i].dispose();
				textures.erase(textures.begin() + i);
				return;
			}
		}
	}
}