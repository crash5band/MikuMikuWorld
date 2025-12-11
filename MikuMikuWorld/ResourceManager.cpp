#include "ResourceManager.h"
#include "IO.h"
#include "BinaryReader.h"
#include "MinMax.h"
#include <filesystem>
#include <sstream>
#include <numeric>

using namespace nlohmann;

namespace MikuMikuWorld
{
	std::vector<Texture> ResourceManager::textures;
	std::vector<Shader*> ResourceManager::shaders;
	std::vector<SpriteTransform> ResourceManager::spriteTransforms;

	int ResourceManager::nextParticleId{ 1 };
	ParticleIdMap ResourceManager::particleIdMap;
	std::map<std::string, int> ResourceManager::effectNameToRootIdMap;

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

	static Effect::KeyFrame readKeyFrame(const json& j)
	{
		float inTangent{}, outTangent{};
		if (jsonIO::keyExists(j, "inTangent"))
		{
			if (j["inTangent"] == "Infinity")
				inTangent = std::numeric_limits<float>::infinity();
			else
				inTangent = j["inTangent"];
		}

		if (jsonIO::keyExists(j, "outTangent"))
		{
			if (j["outTangent"] == "Infinity")
				outTangent = std::numeric_limits<float>::infinity();
			else
				outTangent = j["outTangent"];
		}

		return
		{
			jsonIO::tryGetValue<float>(j, "time", 0),
			jsonIO::tryGetValue<float>(j, "value", 0),
			inTangent,
			outTangent,
			jsonIO::tryGetValue<float>(j, "inWeight", 0),
			jsonIO::tryGetValue<float>(j, "outWeight", 0)
		};
	}

	static Effect::MinMax readMinMax(const json& j)
	{
		Effect::MinMax minmax;
		minmax.constant = j["constant"];
		minmax.min = j["randomMin"];
		minmax.max = j["randomMax"];
		minmax.mode = Effect::MinMaxMode(jsonIO::tryGetValue<int>(j, "mode", 0));

		if (jsonIO::keyExists(j, "curveMin"))
		{
			for (const auto& kJson : j["curveMin"])
				minmax.addKeyFrame(readKeyFrame(kJson), Effect::MinMaxCurve::Min);
		}

		if (jsonIO::keyExists(j, "curveMax"))
		{
			for (const auto& kJson : j["curveMax"])
				minmax.addKeyFrame(readKeyFrame(kJson), Effect::MinMaxCurve::Max);
		}

		minmax.sortKeyFrames();
		return minmax;
	}

	static Effect::MinMax3 readMinMax3(const json& j)
	{
		return
		{
			true,
			readMinMax(j["x"]),
			readMinMax(j["y"]),
			readMinMax(j["z"])
		};
	}

	static Effect::ColorKeyFrame readColorKeyFrame(const json& j)
	{
		return
		{
			jsonIO::tryGetValue<float>(j, "time", 0),
			Color {
				jsonIO::tryGetValue<float>(j, "r", 1.f),
				jsonIO::tryGetValue<float>(j, "g", 1.f),
				jsonIO::tryGetValue<float>(j, "b", 1.f),
				jsonIO::tryGetValue<float>(j, "a", 1.f)
			}
		};
	}

	static Effect::MinMaxColor readMinMaxColor(const json& j)
	{
		Effect::MinMaxColor minmax;
		minmax.mode = (Effect::MinMaxColorMode)jsonIO::tryGetValue<int>(j, "mode", 0);
		minmax.min = jsonIO::tryGetValue(j, "colorMin", Color(1.f, 1.f, 1.f, 1.f));
		minmax.max = jsonIO::tryGetValue(j, "colorMax", Color(1.f, 1.f, 1.f, 1.f));
		minmax.constant = jsonIO::tryGetValue(j, "colorMax", Color(1.f, 1.f, 1.f, 1.f));

		if (jsonIO::keyExists(j, "gradientKeysMin"))
		{
			for (const auto& entry : j["gradientKeysMin"])
				minmax.addKeyFrame(readColorKeyFrame(entry), Effect::MinMaxCurve::Min);
		}

		if (jsonIO::keyExists(j, "gradientKeysMax"))
		{
			for (const auto& entry : j["gradientKeysMax"])
				minmax.addKeyFrame(readColorKeyFrame(entry), Effect::MinMaxCurve::Max);
		}

		minmax.sortKeyFrames();
		return minmax;
	}

	int ResourceManager::readParticle(const json& j)
	{
		Effect::Particle p;
		p.name = j["name"];
		const json& transform = j["transform"];
		Vector3 mainPosition = jsonIO::tryGetValue(transform, "position", Vector3());
		Vector3 mainRotation = jsonIO::tryGetValue(transform, "rotation", Vector3());
		Vector3 mainScale = jsonIO::tryGetValue(transform, "scale", Vector3());

		p.transform.position = { mainPosition.x, mainPosition.y, mainPosition.z, 1 };
		p.transform.rotation = { mainRotation.x, mainRotation.y, mainRotation.z, 1 };
		p.transform.scale = { mainScale.x, mainScale.y, mainScale.z, 1 };

		p.startSize = readMinMax3(j["startSize"]);
		p.startRotation = readMinMax3(j["startRotation"]);
		p.startLifeTime = readMinMax(j["startLifetime"]);
		p.startSpeed = readMinMax(j["startSpeed"]);
		p.startColor = readMinMaxColor(j["startColor"]);
		p.gravityModifier = readMinMax(j["gravityModifier"]);
		p.looping = jsonIO::tryGetValue<bool>(j, "loop", false);
		p.startDelay = readMinMax(j["startDelay"]);
		p.maxParticles = jsonIO::tryGetValue<int>(j, "maxParticles", 1);
		p.scalingMode = (Effect::ScalingMode)jsonIO::tryGetValue<int>(j, "scalingMode", 1);
		p.duration = jsonIO::tryGetValue<float>(j, "duration", 1);
		p.flipRotation = jsonIO::tryGetValue<float>(j, "flipRotation", 0);
		p.simulationSpace = (Effect::TransformSpace)jsonIO::tryGetValue<int>(j, "simulationSpace", 0);
		p.randomSeed = jsonIO::tryGetValue<int>(j, "randomSeed", 0);
		p.useAutoRandomSeed = jsonIO::tryGetValue<bool>(j, "useAutoRandomSeed", true);

		const json& emission = j["emission"];
		p.emission.rateOverTime = readMinMax(emission["rateOverTime"]);
		p.emission.rateOverDistance = readMinMax(emission["rateOverDistance"]);

		const json& emissionBursts = emission["bursts"];
		for (const auto& b : emissionBursts)
		{
			Effect::EmissionBurst burst{};
			burst.count = b["countMax"];
			burst.cycles = b["cycleCount"];
			burst.time = b["time"];
			burst.interval = b["repeatInterval"];
			burst.probability = b["probability"];

			p.emission.bursts.push_back(burst);
		}

		const json& shape = j["shape"];
		const json& shapeTransform = shape["transform"];
		Vector3 emitPosition = jsonIO::tryGetValue(shapeTransform, "position", Vector3());
		Vector3 emitRotation = jsonIO::tryGetValue(shapeTransform, "rotation", Vector3());
		Vector3 emitScale = jsonIO::tryGetValue(shapeTransform, "scale", Vector3());

		p.emission.transform.position = { emitPosition.x, emitPosition.y, emitPosition.z, 1 };
		p.emission.transform.rotation = { emitRotation.x, emitRotation.y, emitRotation.z, 1 };
		p.emission.transform.scale = { emitScale.x, emitScale.y, emitScale.z, 1 };

		p.emission.shape = (Effect::EmissionShape)jsonIO::tryGetValue<int>(shape, "shapeType", 10);
		p.emission.radius = jsonIO::tryGetValue<float>(shape, "radius", 0);
		p.emission.radiusThickness = jsonIO::tryGetValue<float>(shape, "radiusThickness", 0);
		p.emission.angle = jsonIO::tryGetValue<float>(shape, "angle", 0);
		p.emission.arc = jsonIO::tryGetValue<float>(shape, "arc", 0);
		p.emission.arcSpeed = readMinMax(shape["arcSpeed"]);
		p.emission.randomizeDirection = jsonIO::tryGetValue<float>(shape, "randomizeDirection", 0);
		p.emission.randomizePosition = jsonIO::tryGetValue<float>(shape, "randomizePosition", 0);
		p.emission.spherizeDirection = jsonIO::tryGetValue<float>(shape, "spherizeDirection", 0);
		p.emission.arcMode = (Effect::ArcMode)jsonIO::tryGetValue<int>(shape, "arcMode", 0);

		const json& textureSheet = j["textureSheetAnimation"];
		p.textureSplitX = textureSheet["numTilesX"];
		p.textureSplitY = textureSheet["numTilesY"];
		p.startFrame = readMinMax(textureSheet["startFrame"]);
		p.frameOverTime = readMinMax(textureSheet["frameOverTime"]);

		const json& renderer = j["renderer"];
		p.pivot = jsonIO::tryGetValue(renderer, "pivot", Vector3());
		p.order = jsonIO::tryGetValue<int>(renderer, "order", 50);
		p.speedScale = jsonIO::tryGetValue<float>(renderer, "speedScale");
		p.lengthScale = jsonIO::tryGetValue<float>(renderer, "lengthScale");
		p.renderMode = (Effect::RenderMode)jsonIO::tryGetValue<int>(renderer, "mode");
		p.alignment = (Effect::AlignmentMode)jsonIO::tryGetValue<int>(renderer, "alignment");

		float blend = jsonIO::tryGetValue<float>(j["customData"], "blend", 0);
		p.blend = blend < 0.5 ? Effect::BlendMode::Typical : Effect::BlendMode::Additive;

		if (jsonIO::keyExists(j, "velocityOverLifetime"))
		{
			const json& velocityOverLifetime = j["velocityOverLifetime"];
			p.velocityOverLifetime = readMinMax3(velocityOverLifetime["linear"]);
			p.velocitySpace = (Effect::TransformSpace)jsonIO::tryGetValue<int>(velocityOverLifetime, "space", 0);
			p.speedModifier = readMinMax(velocityOverLifetime["speedModifier"]);
		}
		else
		{
			p.speedModifier.mode = Effect::MinMaxMode::Constant;
			p.speedModifier.constant = 1.f;
		}

		if (jsonIO::keyExists(j, "limitVelocityOverLifetime"))
		{
			const json& limitVelocityOverLifetime = j["limitVelocityOverLifetime"];
			p.limitVelocityOverLifetime = readMinMax3(limitVelocityOverLifetime["speed"]);
			p.limitVelocityDampen = limitVelocityOverLifetime["dampen"];
		}

		if (jsonIO::keyExists(j, "forceOverLifetime"))
		{
			const json& forceOverLifetime = j["forceOverLifetime"];
			p.forceOverLifetime = readMinMax3(forceOverLifetime["value"]);
			p.forceSpace = (Effect::TransformSpace)jsonIO::tryGetValue<int>(forceOverLifetime, "space", 0);
		}

		if (jsonIO::keyExists(j, "colorOverLifetime"))
		{
			const json& colorOverLifetime = j["colorOverLifetime"];
			p.colorOverLifetime = readMinMaxColor(colorOverLifetime);
		}

		if (jsonIO::keyExists(j, "sizeOverLifetime"))
		{
			p.sizeOverLifetime = readMinMax3(j["sizeOverLifetime"]);
		}

		if (jsonIO::keyExists(j, "rotationOverLifetime"))
		{
			p.rotationOverLifetime = readMinMax3(j["rotationOverLifetime"]);
		}

		p.ID = nextParticleId++;
		for (const auto& child : j["children"])
		{
			int childId = readParticle(child);
			p.children.push_back(childId);
		}

		particleIdMap[p.ID] = p;
		return p.ID;
	}

	int ResourceManager::loadParticleEffect(const std::string& filename)
	{
		std::string effectName = IO::File::getFilenameWithoutExtension(filename);
		std::wstring wFilename = IO::mbToWideStr(filename);
		std::ifstream particleFile(wFilename);
		
		json effectJson{};
		particleFile >> effectJson;
		particleFile.close();

		int effectRootId = readParticle(effectJson);
		effectNameToRootIdMap[effectName] = effectRootId;

		return effectRootId;
	}

	Effect::Particle& ResourceManager::getParticleEffect(int id)
	{
		return particleIdMap.at(id);
	}

	int ResourceManager::getRootParticleIdByName(std::string name)
	{
		auto it = effectNameToRootIdMap.find(name);
		if (it == effectNameToRootIdMap.end())
			return 0;

		return it->second;
	}

	void ResourceManager::removeAllParticleEffects()
	{
		particleIdMap.clear();
		effectNameToRootIdMap.clear();
		nextParticleId = 1;
	}
}