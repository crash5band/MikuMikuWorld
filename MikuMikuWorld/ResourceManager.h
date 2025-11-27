#pragma once
#include <vector>
#include "Rendering/Texture.h"
#include "Rendering/Shader.h"
#include "PreviewEngine.h"
#include "Particle.h"
#include "JsonIO.h"

namespace MikuMikuWorld
{
	typedef std::map<int, Effect::Particle> ParticleIdMap;

	class ResourceManager
	{
	public:
		static std::vector<Texture> textures;
		static std::vector<Shader*> shaders;
		static std::vector<SpriteTransform> spriteTransforms;

		static void loadTexture(const std::string& filename, TextureFilterMode minFilter = TextureFilterMode::Linear, TextureFilterMode magFilter = TextureFilterMode::Linear);
		static int getTexture(const std::string& name);
		static int getTextureByFilename(const std::string& filename);

		static void loadShader(const std::string& filename);
		static int getShader(const std::string& name);

		static void loadTransforms(const std::string& filename);

		static void disposeTexture(int texID);

		static int loadParticleEffect(const std::string& filename);
		static Effect::Particle& getParticleEffect(int id);
		static int getRootParticleIdByName(std::string name);

		static void removeAllParticleEffects();

	private:
		static int nextParticleId;
		static ParticleIdMap particleIdMap;
		static std::map<std::string, int> effectNameToRootIdMap;

		static int readParticle(const nlohmann::json& j);
	};
}

