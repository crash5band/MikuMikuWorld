#pragma once
#include <string>
#include "json.hpp"
#include "Math.h"
#include "Color.h"

namespace MikuMikuWorld
{
	struct KeyConfiguration
	{
		std::string commandName;
		std::vector<std::string> keyBindings;
	};

	class ApplicationConfiguration
	{
	private:
		bool keyExists(const nlohmann::json&, const char* key);
		int tryGetInt(const nlohmann::json&, const char* key, int def);
		float tryGetFloat(const nlohmann::json&, const char* key, float def);
		bool tryGetBool(const nlohmann::json&, const char* key, bool def);
		Vector2 tryGetVector2(const nlohmann::json&, const char* key);
		Color tryGetColor(const nlohmann::json&, const char* key);
		std::string tryGetString(const nlohmann::json&, const char* key);

	public:
		std::string version;
		Vector2 windowPos;
		Vector2 windowSize;
		bool maximized;
		bool vsync;
		int accentColor;
		Color userColor;

		int timelineWidth;
		int notesHeight;
		int division;
		float zoom;
		float timelineTransparency;
		float backgroundBrightness;
		bool useSmoothScrolling;
		float smoothScrollingTime;
		std::string scrollMode;

		bool autoSaveEnabled;
		int autoSaveInterval;
		int autoSaveMaxCount;

		float masterVolume;
		float bgmVolume;
		float seVolume;

		std::unordered_map<std::string, KeyConfiguration> keyConfigMap;

		ApplicationConfiguration();

		void read(const std::string& filename);
		void write(const std::string& filename);
		void restoreDefault();
	};
}