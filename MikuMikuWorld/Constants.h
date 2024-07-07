#pragma once

namespace MikuMikuWorld
{
	constexpr int TICKS_PER_BEAT = 480;

	constexpr float MEASURE_WIDTH = 30.0f;
	constexpr int MIN_LANE_WIDTH = 24;
	constexpr int MAX_LANE_WIDTH = 36;
	constexpr int MIN_NOTES_HEIGHT = 24;
	constexpr int MAX_NOTES_HEIGHT = 36;

	constexpr int MIN_TIME_SIGNATURE = 1;
	constexpr int MAX_TIME_SIGNATURE_NUMERATOR = 32;
	constexpr int MAX_TIME_SIGNATURE_DENOMINATOR = 64;
	constexpr float MIN_BPM = 10;
	constexpr float MAX_BPM = 10000;

	constexpr const char* NOTES_TEX = "notes1";
	constexpr const char* CC_NOTES_TEX = "notes2";
	constexpr const char* HOLD_PATH_TEX = "longNoteLine";
	constexpr const char* TOUCH_LINE_TEX = "touchLine_eff";
	constexpr const char* GUIDE_COLORS_TEX = "guideColors";
	constexpr const char* APP_CONFIG_FILENAME = "app_config.json";
	constexpr const char* IMGUI_CONFIG_FILENAME = "imgui_config.ini";

	constexpr const char* SUS_EXTENSION = ".sus";
	constexpr const char* USC_EXTENSION = ".usc";
	constexpr const char* MMWS_EXTENSION = ".mmws";
	constexpr const char* CC_MMWS_EXTENSION = ".ccmmws";
}
