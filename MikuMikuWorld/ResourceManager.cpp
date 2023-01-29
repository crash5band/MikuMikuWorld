#include "ResourceManager.h"
#include "StringOperations.h"
#include <filesystem>

namespace MikuMikuWorld
{
	std::vector<Texture> ResourceManager::textures;
	std::vector<Shader*> ResourceManager::shaders;

	void ResourceManager::loadTexture(const std::string filename)
	{
		if (!File::exists(filename))
		{
			printf("ERROR: ResourceManager::loadTexture() Could not find texture file %s\n", filename.c_str());
			return;
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
		Shader* s = new Shader(File::getFilenameWithoutExtension(filename), filename);
		shaders.push_back(s);
	}

	int ResourceManager::getShader(const std::string& name)
	{
		for (int i = 0; i < shaders.size(); ++i)
			if (shaders[i]->getName() == name)
				return i;

		return -1;
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