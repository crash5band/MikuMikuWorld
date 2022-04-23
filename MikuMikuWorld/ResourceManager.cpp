#include "ResourceManager.h"
#include "StringOperations.h"
#include <filesystem>

namespace MikuMikuWorld
{
	std::vector<Texture> ResourceManager::textures;
	std::vector<Shader*> ResourceManager::shaders;

	void ResourceManager::loadTexture(const std::string filename)
	{
		std::wstring wFilename = mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
		{
			printf("ERROR: ResourceManager::loadTexture() Could not find texture file %ws\n", wFilename.c_str());
			//return;
		}

		Texture tex(filename);
		textures.push_back(tex);
	}

	int ResourceManager::getTexture(const std::string& name)
	{
		for (int i = 0; i < textures.size(); ++i)
			if (textures[i].getName() == name)
				return i;

		return -1;
	}

	void ResourceManager::loadShader(const std::string& filename)
	{
		Shader* s = new Shader("basic2d", filename);
		shaders.push_back(s);
	}

	int ResourceManager::getShader(const std::string& name)
	{
		for (int i = 0; i < shaders.size(); ++i)
			if (shaders[i]->getName() == name)
				return i;

		return -1;
	}

}