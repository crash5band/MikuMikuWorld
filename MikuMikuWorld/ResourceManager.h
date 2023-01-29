#pragma once
#include <vector>
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"

namespace MikuMikuWorld
{
	class ResourceManager
	{
	public:
		static std::vector<Texture> textures;
		static std::vector<Shader*> shaders;

		static void loadTexture(const std::string filename);
		static int getTexture(const std::string& name);

		static void loadShader(const std::string& filename);
		static int getShader(const std::string& name);

		static void disposeTexture(int texID);
	};
}

