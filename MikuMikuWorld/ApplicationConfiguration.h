#pragma once
#include <string>
#include "UI.h"
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
	public:
		std::string version;
		Vector2 windowPos;
		Vector2 windowSize;
		bool maximized;
		bool vsync;
		int accentColor;
		Color userColor;
		BaseTheme baseTheme;

		int timelineWidth;
		int notesHeight;
		int division;
		float zoom;
		float laneOpacity;
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