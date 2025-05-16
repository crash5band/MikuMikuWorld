#include "ApplicationConfiguration.h"
#include "IO.h"
#include "JsonIO.h"
#include "Constants.h"
#include <filesystem>
#include <fstream>

using namespace nlohmann;

namespace MikuMikuWorld
{
	ApplicationConfiguration config{};
	constexpr const char* CONFIG_VERSION{ "1.12.0" };

	ApplicationConfiguration::ApplicationConfiguration() : version{ CONFIG_VERSION }
	{
		recentFiles.reserve(maxRecentFilesEntries);
		restoreDefault();
	}

	void ApplicationConfiguration::read(const std::string& filename)
	{
		std::wstring wFilename = IO::mbToWideStr(filename);
		if (!std::filesystem::exists(wFilename))
			return;

		std::ifstream configFile(wFilename);
		json config;
		configFile >> config;
		configFile.close();

		version = jsonIO::tryGetValue<std::string>(config, "version", "1.0");
		language = jsonIO::tryGetValue<std::string>(config, "language", "auto");
		debugEnabled = jsonIO::tryGetValue<bool>(config, "debug", false);

		if (jsonIO::keyExists(config, "window"))
		{
			const json& window = config["window"];
			maximized = jsonIO::tryGetValue<bool>(window, "maximized", false);
			vsync = jsonIO::tryGetValue<bool>(window, "vsync", true);
			showFPS = jsonIO::tryGetValue<bool>(window, "show_fps", false);

			windowPos = jsonIO::tryGetValue(window, "position", Vector2{});
			if (windowPos.x <= 0) windowPos.x = 150;
			if (windowPos.y <= 0) windowPos.y = 100;

			windowSize = jsonIO::tryGetValue(window, "size", Vector2{});
			if (windowSize.x <= 0 || windowSize.y <= 0)
			{
				windowSize.x = 1000;
				windowSize.y = 800;
			}
		}

		if (jsonIO::keyExists(config, "timeline"))
		{
			timelineWidth = std::clamp(jsonIO::tryGetValue<int>(config["timeline"], "lane_width", 26), MIN_LANE_WIDTH, MAX_LANE_WIDTH);
			notesHeight = std::clamp(jsonIO::tryGetValue<int>(config["timeline"], "notes_height", 26), MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT);
			matchTimelineSizeToScreen = jsonIO::tryGetValue<bool>(config["timeline"], "match_timeline_size_to_window", false);
			matchNotesSizeToTimeline = jsonIO::tryGetValue<bool>(config["timeline"], "match_notes_size_to_timeline", true);

			division = jsonIO::tryGetValue<int>(config["timeline"], "division", 8);
			zoom = jsonIO::tryGetValue<float>(config["timeline"], "zoom", 2.0f);
			laneOpacity = jsonIO::tryGetValue<float>(config["timeline"], "lane_opacity", 0.0f);
			backgroundBrightness = jsonIO::tryGetValue<float>(config["timeline"], "background_brightness", 0.5f);
			drawBackground = jsonIO::tryGetValue<bool>(config["timeline"], "draw_background", true);
			backgroundImage = jsonIO::tryGetValue<std::string>(config["timeline"], "background_image", "");

			useSmoothScrolling = jsonIO::tryGetValue<bool>(config["timeline"], "smooth_scrolling_enable", true);
			smoothScrollingTime = jsonIO::tryGetValue<float>(config["timeline"], "smooth_scrolling_time", 48.0f);
			scrollSpeedNormal = jsonIO::tryGetValue<float>(config["timeline"], "scroll_speed_normal", 2.0f);
			scrollSpeedShift = jsonIO::tryGetValue<float>(config["timeline"], "scroll_speed_fast", 5.0f);

			hideStepOutlinesInPlayback = jsonIO::tryGetValue<bool>(config["timeline"], "hide_step_outlines_in_playback", true);
			drawWaveform = jsonIO::tryGetValue<bool>(config["timeline"], "draw_waveform", true);
			returnToLastSelectedTickOnPause = jsonIO::tryGetValue<bool>(config["timeline"], "return_to_last_tick_on_pause", false);
			cursorPositionThreshold = jsonIO::tryGetValue<float>(config["timeline"], "cursor_position_threshold", 0.5f);
			stopAtEnd = jsonIO::tryGetValue<bool>(config["timeline"], "stop_at_end", true);
		}

		if (jsonIO::keyExists(config, "theme"))
		{
			accentColor = jsonIO::tryGetValue<int>(config["theme"], "accent_color", 1);
			userColor = jsonIO::tryGetValue(config["theme"], "user_color", Color{});
			baseTheme = (BaseTheme)jsonIO::tryGetValue<int>(config["theme"], "base_theme", 0);
		}

		if (jsonIO::keyExists(config, "save"))
		{
			autoSaveEnabled	= jsonIO::tryGetValue<bool>(config["save"], "auto_save_enabled", true);
			autoSaveInterval = jsonIO::tryGetValue<int>(config["save"], "auto_save_interval", 5);
			autoSaveMaxCount = jsonIO::tryGetValue<int>(config["save"], "auto_save_max_count", 100);
			lastSelectedExportIndex = jsonIO::tryGetValue<int>(config["save"], "last_export_option", 0);
		}

		if (jsonIO::keyExists(config, "audio"))
		{
			seProfileIndex = jsonIO::tryGetValue<int>(config["audio"], "se_profile", 0);
			masterVolume	= std::clamp(jsonIO::tryGetValue<float>(config["audio"], "master_volume", 1.0f), 0.0f, 1.0f);
			bgmVolume		= std::clamp(jsonIO::tryGetValue<float>(config["audio"], "bgm_volume", 1.0f), 0.0f, 1.0f);
			seVolume		= std::clamp(jsonIO::tryGetValue<float>(config["audio"], "se_volume", 1.0f), 0.0f, 1.0f);
		}

		if (jsonIO::keyExists(config, "input") && jsonIO::keyExists(config["input"], "bindings"))
		{
			for (auto& [key, value] : config["input"]["bindings"].items())
			{
				for (int i = 0; i < sizeof(bindings) / sizeof(MultiInputBinding*); ++i)
				{
					if (bindings[i]->name == key)
					{
						int keysCount = std::min(value.size(), bindings[i]->bindings.size());
						for (int k = 0; k < keysCount; ++k)
							bindings[i]->bindings[k] = FromSerializedString(value[k]);

						bindings[i]->count = keysCount;
					}
				}
			}
		}

		if (jsonIO::arrayHasData(config, "recent_files"))
		{
			const json& recentFilesJson = config["recent_files"];
			const size_t count = std::min(recentFilesJson.size(), maxRecentFilesEntries);
			recentFiles.insert(recentFiles.end(), recentFilesJson.begin(), recentFilesJson.begin() + count);
		}
	}

	void ApplicationConfiguration::write(const std::string& filename)
	{
		json config;

		// update to latest version
		config["version"] = CONFIG_VERSION;
		config["language"] = language;
		config["debug"] = debugEnabled;
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
		config["window"]["show_fps"] = showFPS;

		config["timeline"] = {
			{"lane_width", timelineWidth},
			{"notes_height", notesHeight},
			{"match_timeline_size_to_window", matchTimelineSizeToScreen},
			{"match_notes_size_to_timeline", matchNotesSizeToTimeline},
			{"division", division},
			{"zoom", zoom},
			{"lane_opacity", laneOpacity},
			{"background_brightness", backgroundBrightness},
			{"draw_background", drawBackground},
			{"background_image", backgroundImage},
			{"smooth_scrolling_enable", useSmoothScrolling},
			{"smooth_scrolling_time", smoothScrollingTime},
			{"scroll_speed_normal", scrollSpeedNormal},
			{"scroll_speed_fast", scrollSpeedShift},
			{"hide_step_outlines_in_playback", hideStepOutlinesInPlayback},
			{"draw_waveform", drawWaveform},
			{"return_to_last_tick_on_pause", returnToLastSelectedTickOnPause},
			{"cursor_position_threshold", cursorPositionThreshold},
			{"stop_at_end", stopAtEnd}
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
			},
			{ "base_theme", static_cast<int>(baseTheme) }
		};

		config["save"] = {
			{"auto_save_enabled", autoSaveEnabled},
			{"auto_save_interval", autoSaveInterval},
			{"auto_save_max_count", autoSaveMaxCount},
			{"last_export_option", lastSelectedExportIndex}
		};

		config["audio"] = {
			{"se_profile", seProfileIndex},
			{"master_volume", masterVolume},
			{"bgm_volume", bgmVolume},
			{"se_volume", seVolume}
		};

		json keyBindings;
		for (const auto& binding : bindings)
		{
			json keys;
			for (int k = 0; k < binding->count; ++k)
			{
				if (binding->bindings[k].keyCode != ImGuiKey_None)
					keys.push_back(ToSerializedString(binding->bindings[k]));
			}

			keyBindings[binding->name] = keys;
		}

		config["input"] = {
			{"bindings", keyBindings}
		};

		config["recent_files"] = recentFiles;

		std::wstring wFilename = IO::mbToWideStr(filename);
		std::ofstream configFile(wFilename);
		configFile << std::setw(4) << config;
		configFile.flush();
		configFile.close();
	}

	void ApplicationConfiguration::restoreDefault()
	{
		windowPos = Vector2(150, 100);
		windowSize = Vector2(1000, 800);
		maximized = false;
		vsync = true;
		accentColor = 1;
		userColor = Color(0.2f, 0.2f, 0.2f, 1.0f);
		language = "auto";

		timelineWidth = 26;
		notesHeight = 26;
		matchNotesSizeToTimeline = true;
		division = 8;
		zoom = 2.0f;
		laneOpacity = 0.0f;
		backgroundBrightness = 0.5f;
		drawBackground = true;
		backgroundImage = "";
		useSmoothScrolling = true;
		smoothScrollingTime = 48.0f;
		scrollSpeedNormal = 2.0f;
		scrollSpeedShift = 5.0f;
		cursorPositionThreshold = 0.5;
		drawWaveform = true;
		followCursorInPlayback = true;
		returnToLastSelectedTickOnPause = false;
		hideStepOutlinesInPlayback = true;
		stopAtEnd = true;

		autoSaveEnabled = true;
		autoSaveInterval = 5;
		autoSaveMaxCount = 100;
		lastSelectedExportIndex = 0;

		seProfileIndex = 0;
		masterVolume = 1.0f;
		bgmVolume = 1.0f;
		seVolume = 1.0f;

		debugEnabled = false;
	}
}