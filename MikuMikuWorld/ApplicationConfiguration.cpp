#include "ApplicationConfiguration.h"
#include "StringOperations.h"
#include <filesystem>
#include <fstream>

using namespace nlohmann;

namespace MikuMikuWorld
{
	constexpr const char* CONFIG_VERSION{ "1.3.1" };

	ApplicationConfiguration::ApplicationConfiguration() : version{ CONFIG_VERSION }
	{
		restoreDefault();
	}

	bool ApplicationConfiguration::keyExists(const json& js, const char* key)
	{
		return (js.find(key) != js.end());
	}

	int ApplicationConfiguration::tryGetInt(const json& js, const char* key, int def)
	{
		if (keyExists(js, key))
			return js[key].is_number() ? (int)js[key] : def;
		
		return def;
	}

	float ApplicationConfiguration::tryGetFloat(const json& js, const char* key, float def)
	{
		if (keyExists(js, key))
			return js[key].is_number() ? (float)js[key] : def;

		return def;
	}

	bool ApplicationConfiguration::tryGetBool(const json& js, const char* key, bool def)
	{
		if (keyExists(js, key))
			return js[key].is_boolean() ? (bool)js[key] : def;

		return def;
	}

	Vector2 ApplicationConfiguration::tryGetVector2(const json& js, const char* key)
	{
		Vector2 v;
		if (keyExists(js, key))
		{
			v.x = tryGetFloat(js[key], "x", 0.0f);
			v.y = tryGetFloat(js[key], "y", 0.0f);
		}

		return v;
	}

	Color ApplicationConfiguration::tryGetColor(const json& js, const char* key)
	{
		Color c;
		if (keyExists(js, key))
		{
			c.r = tryGetFloat(js[key], "r", 1.0f);
			c.g = tryGetFloat(js[key], "g", 1.0f);
			c.b = tryGetFloat(js[key], "b", 1.0f);
			c.a = tryGetFloat(js[key], "a", 1.0f);
		}

		return c;
	}

	std::string ApplicationConfiguration::tryGetString(const json& js, const char* key)
	{
		if (keyExists(js, key))
			return js[key].is_string() ? js[key] : "";

		return "";
	}

	void ApplicationConfiguration::read(const std::string& filename)
	{
		std::wstring wFilename = mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
			return;

		std::ifstream configFile(wFilename);
		json config;
		configFile >> config;
		configFile.close();

		version = tryGetString(config, "version");
		if (!version.size())
			version = "1.0.0";

		if (keyExists(config, "window"))
		{
			const json& window = config["window"];
			maximized = tryGetBool(window, "maximized", false);
			vsync = tryGetBool(window, "vsync", true);

			windowPos = tryGetVector2(window, "position");
			if (windowPos.x <= 0) windowPos.x = 150;
			if (windowPos.y <= 0) windowPos.y = 100;

			windowSize = tryGetVector2(window, "size");
			if (windowSize.x <= 0 || windowSize.y <= 0)
			{
				windowSize.x = 1200;
				windowSize.y = 800;
			}
		}

		if (keyExists(config, "timeline"))
		{
			timelineWidth = tryGetInt(config["timeline"], "lane_width", 35);
			notesHeight	= tryGetInt(config["timeline"], "notes_height", 45);
			division = tryGetInt(config["timeline"], "division", 8);
			zoom = tryGetFloat(config["timeline"], "zoom", 2.0f);
			timelineTransparency = tryGetFloat(config["timeline"], "lane_transparency", 0.8f);
			backgroundBrightness = tryGetFloat(config["timeline"], "background_brightness", 0.4f);

			useSmoothScrolling	= tryGetBool(config["timeline"], "smooth_scrolling_enable", true);
			smoothScrollingTime = tryGetFloat(config["timeline"], "smooth_scrolling_time", 67.0f);

			scrollMode = tryGetString(config["timeline"], "scroll_mode");
			if (!scrollMode.size())
				scrollMode = "follow_cursor";
		}

		if (keyExists(config, "theme"))
		{
			accentColor = tryGetInt(config["theme"], "accent_color", 1);
			userColor	= tryGetColor(config["theme"], "user_color");
		}

		if (keyExists(config, "save"))
		{
			autoSaveEnabled  = tryGetBool(config["save"], "auto_save_enabled", true);
			autoSaveInterval = tryGetInt(config["save"], "auto_save_interval", 5);
			autoSaveMaxCount = tryGetInt(config["save"], "auto_save_max_count", 100);
		}

		if (keyExists(config, "audio"))
		{
			masterVolume	= std::clamp(tryGetFloat(config["audio"], "master_volume", 0.8f), 0.0f, 1.0f);
			bgmVolume		= std::clamp(tryGetFloat(config["audio"], "bgm_volume", 1.0f), 0.0f, 1.0f);
			seVolume		= std::clamp(tryGetFloat(config["audio"], "se_volume", 1.0f), 0.0f, 1.0f);
		}
	}

	void ApplicationConfiguration::write(const std::string& filename)
	{
		json config;

		// update to latest version
		config["version"] =	CONFIG_VERSION;
		config["window"]["position"] = { 
			{"x", windowPos.x},
			{"y", windowPos.y} 
		};

		config["window"]["size"] = {
			{"x", windowSize.x},
			{"y", windowSize.y}
		};

		config["window"]["maximized"] = maximized;
		config["window"]["vsync"] = vsync;

		config["timeline"] = {
			{"lane_width", timelineWidth},
			{"notes_height", notesHeight},
			{"division", division},
			{"zoom", zoom},
			{"lane_transparency", timelineTransparency},
			{"background_brightness", backgroundBrightness},
			{"smooth_scrolling_enable", useSmoothScrolling},
			{"smooth_scrolling_time", smoothScrollingTime},
			{"scroll_mode", scrollMode}
		};

		config["theme"] = {
			{"accent_color", accentColor},
			{"user_color",
				{ 
					{"r", userColor.r},
					{"g", userColor.g},
					{"b", userColor.b},
					{"a", userColor.a}
				}
			}
		};

		config["save"] = {
			{"auto_save_enabled", autoSaveEnabled},
			{"auto_save_interval", autoSaveInterval},
			{"auto_save_max_count", autoSaveMaxCount}
		};

		config["audio"] = {
			{"master_volume", masterVolume},
			{"bgm_volume", bgmVolume},
			{"se_volume", seVolume}
		};

		std::wstring wFilename = mbToWideStr(filename);
		std::ofstream configFile(wFilename);
		configFile << std::setw(4) << config;
		configFile.flush();
		configFile.close();
	}

	void ApplicationConfiguration::restoreDefault()
	{
		windowPos = Vector2(150, 100);
		windowSize = Vector2(1200, 800);
		maximized = false;
		vsync = true;
		accentColor = 1;
		userColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
		
		timelineWidth = 35;
		notesHeight = 45;
		division = 8;
		zoom = 2.0f;
		timelineTransparency = 0.8f;
		backgroundBrightness = 0.4f;
		useSmoothScrolling = true;
		smoothScrollingTime = 67.0f;
		scrollMode = "follow_cursor";

		autoSaveEnabled = true;
		autoSaveInterval = 5;
		autoSaveMaxCount = 100;

		masterVolume = 0.8f;
		bgmVolume = 1.0f;
		seVolume = 1.0f;
	}
}