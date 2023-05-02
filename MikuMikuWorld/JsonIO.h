#pragma once
#include "Math.h"
#include "Color.h"
#include "Score.h"
#include <json.hpp>
#include <unordered_set>

namespace mmw = MikuMikuWorld;

namespace jsonIO
{
	static bool keyExists(const nlohmann::json& js, const char* key)
	{
		return (js.find(key) != js.end());
	}

	template<typename T>
	T tryGetValue(const nlohmann::json& js, const char* key, T def = {})
	{
		return keyExists(js, key) ? (T)js[key] : def;
	}

	static mmw::Vector2 tryGetValue(const nlohmann::json& js, const char* key, mmw::Vector2 def)
	{
		mmw::Vector2 v = def;
		if (keyExists(js, key))
		{
			v.x = tryGetValue<float>(js[key], "x", def.x);
			v.y = tryGetValue<float>(js[key], "y", def.y);
		}

		return v;
	}

	static mmw::Color tryGetValue(const nlohmann::json& js, const char* key, mmw::Color def)
	{
		mmw::Color c = def;
		if (keyExists(js, key))
		{
			c.r = tryGetValue<float>(js[key], "r", def.r);
			c.g = tryGetValue<float>(js[key], "g", def.g);
			c.b = tryGetValue<float>(js[key], "b", def.b);
			c.a = tryGetValue<float>(js[key], "a", def.a);
		}

		return c;
	}

	mmw::Note jsonToNote(const nlohmann::json& data, mmw::NoteType type);

	nlohmann::json noteToJson(const mmw::Note& note);

	nlohmann::json noteSelectionToJson(const mmw::Score& score, const std::unordered_set<int>& selection, int baseTick);
}