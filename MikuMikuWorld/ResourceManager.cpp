#include "ResourceManager.h"
#include "IO.h"
#include "BinaryReader.h"
#include <filesystem>
#include <sstream>
#include <numeric>

namespace MikuMikuWorld
{
	std::vector<Texture> ResourceManager::textures;
	std::vector<Shader*> ResourceManager::shaders;
	std::vector<SpriteTransform> ResourceManager::spriteTransforms;
	std::vector<ParticleEffect> ResourceManager::particleEffects;

	void ResourceManager::loadTexture(const std::string& filename, TextureFilterMode minFilter, TextureFilterMode magFilter)
	{
		if (!IO::File::exists(filename))
		{
			fprintf(stderr, "ERROR: ResourceManager::loadTexture() Could not find texture file %s\n", filename.c_str());
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

	void ResourceManager::loadTransforms(const std::string &filename)
	{
		if (!IO::File::exists(filename))
		{
			fprintf(stderr, "ERROR: ResourceManager::loadTransforms() Could not find the file %s\n", filename.c_str());
			return;
		}
		int idx = 0;
		float transform[8 * 8];
		IO::File file(filename, IO::FileMode::Read);
		
		std::string txt = file.readAllText();
		if (txt.empty())
		{
			return;
		}
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
				spriteTransforms.emplace_back(transform);
			}
		}
		if (!ss.eof())
		{
			ss.clear();
			auto invalid_pos = ss.tellg();
			char invalid = ss.get();
			std::string msg = IO::formatString(
				"ERROR: ResourceManager::loadTransforms()\n"
				"Unexpected characters '%c' at position %ld while reading \"%s\"\n", invalid, (long)invalid_pos, filename.c_str());
			throw std::runtime_error(msg);
		}
		if (idx != 0)
		{
			throw std::runtime_error(
				"ERROR: ResourceManager::loadTransforms()\n"
				"Incompleted transform declaration!\n"
			);
		}
	}

	void ResourceManager::loadParticleEffects(const std::string &filename)
	{
		// [PTE] structure
		// + uint32: "pte" signature
		// + uint32: count of [ParticleEffect]
		// + n * [ParticleEffect]: particle effect list
		// [ParticleEffect] structure
		// + uint32: count of [ParticleEffectGroup]
		// + n * [ParticleEffectGroup]: particle effect group list
		// [ParticleEffectGroup]
		// + uint32: amount of times the particles in the group to be spawned
		// + uint32: count of [Particle]
		// + n * [Particle]: particle list
		// [Particle] structure
		// + uint32: sprite id
		// + [Color] = uint32: 8 bit r,g,b color value; a is ignored
		// + float: start time (%)
		// + float: duration (%)
		// + 6 * [ParticleProperty]:
		// + 3 * 6 * uint16: flags for the 3 coeffecient matrices
		// + n * float: values of the coeffecient matrices by xy, wh, ta
		// [ParticleProperty] structure
		// + float: from
		// + float: to
		// + uint16: easing
		if (!IO::File::exists(filename))	
		{
			fprintf(stderr, "ERROR: ResourceManager::loadParticleEffects() Could not find the file %s\n", filename.c_str());
			return;
		}
		IO::BinaryReader reader(filename);
		if (!reader.isStreamValid())
			return;
		if (reader.readString() != "pte")
			throw std::runtime_error("ERROR: ResourceManager::loadParticleEffects()\n Invalid particle effect file.\n");
	
		const auto readCoeff = [](IO::BinaryReader& reader, uint16_t* flags) -> PropertyCoeff
		{
			const auto readMat = [](IO::BinaryReader& reader, uint16_t flag) -> std::unique_ptr<DirectX::XMMATRIX>
			{
				if (flag)
				{
					float buff[16] = {0};
					for (size_t i = 0; i < 16 && flag; flag >>= 1, i++)
					{
						if (flag & 1)
							buff[i] = reader.readSingle();
					}
					return std::make_unique<DirectX::XMMATRIX>(buff);
				}
				return nullptr;
			};
			return
			{
				readMat(reader, flags[0]),
				readMat(reader, flags[1]),
				readMat(reader, flags[2]),
				readMat(reader, flags[3]),
				readMat(reader, flags[4]),
				readMat(reader, flags[5])
			};
		};
		uint32_t effectCount = reader.readInt32();
		particleEffects.reserve(effectCount);
		for (uint32_t e = 0; e < effectCount; e++)
		{
			ParticleEffect particleEffect;
			uint32_t groupCount = reader.readInt32();
			particleEffect.groupSizes.reserve(groupCount);
			for (uint32_t g = 0; g < groupCount; g++)
			{
				uint32_t particleSpawn = reader.readInt32();
				particleEffect.groupSizes.push_back(particleSpawn);
				uint32_t particleCount = reader.readInt32();
				for (uint32_t p = 0; p < particleCount; p++)
				{
					uint32_t spriteId = reader.readInt32();
					uint32_t color = reader.readInt32();
					float start = reader.readSingle();
					float duration = reader.readSingle();
					std::array<ParticleProperty, 8> props;
					for (auto& prop : props)
					{
						float from = reader.readSingle();
						float to = reader.readSingle();
						Engine::Easing easeType = (Engine::Easing)reader.readInt16();
						prop = {from, to, easeType};
					}
					std::array<uint16_t, 3 * 8> flags;
					for (auto& flag : flags)
						flag = reader.readInt16();
					PropertyCoeff xy = readCoeff(reader, flags.data()); 
					PropertyCoeff wh = readCoeff(reader, flags.data() + 6);
					PropertyCoeff ta = readCoeff(reader, flags.data() + 12);
					PropertyCoeff u1u2 = readCoeff(reader, flags.data() + 18);
					
					float order = DirectX::XMVectorGetZ(u1u2.r1_4->r[0]);

					particleEffect.particles.push_back(Particle{
						(int)g,
						(int)spriteId,
						Color{(uint8_t)(color >> 24), (uint8_t)(color >> 16), (uint8_t)(color >> 8)},
						start,
						duration,
						(int)order,
						props,
						std::move(xy),
						std::move(wh),
						std::move(ta),
						std::move(u1u2)
					});
				}
			}
			particleEffect.particles.shrink_to_fit();
			particleEffects.push_back(std::move(particleEffect));
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