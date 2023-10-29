#pragma once
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include <vector>

namespace MikuMikuWorld
{
	class ResourceManager
	{
	  public:
		static std::vector<Texture> textures;
		static std::vector<Shader*> shaders;

		static void loadTexture(const std::string filename);
		static int getTexture(const std::string& name);
		static int getTextureByFilename(const std::string& filename);

		static void loadShader(const std::string& filename);
		static int getShader(const std::string& name);

		static void disposeTexture(int texID);
	};
}
