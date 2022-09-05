#pragma once

namespace MikuMikuWorld
{
	constexpr int TICKS_PER_BEAT = 480;

	constexpr int NOTES_X_SLICE = 83;
	constexpr int NOTES_SIDE_WIDTH = 30;
	constexpr float NOTES_SLICE_WIDTH = 15;
	constexpr float NOTES_X_ADJUST = 4;
	constexpr float HOLD_X_SLICE = 32;
	constexpr float HIGHLIGHT_SLICE = 30;
	constexpr float HIGHLIGHT_HEIGHT = 25;

	constexpr int MIN_NOTE_WIDTH = 1;
	constexpr int MAX_NOTE_WIDTH = 12;
	constexpr int MIN_LANE = 0;
	constexpr int MAX_LANE = 11;
	constexpr int NUM_LANES = 12;
	constexpr float TICK_HEIGHT = 0.15f;
	constexpr float MIN_ZOOM = 0.25f;
	constexpr float MAX_ZOOM = 10.0f;
	constexpr float MEASURE_WIDTH = 30.0f;
	constexpr int MIN_LANE_WIDTH = 30;
	constexpr int MAX_LANE_WIDTH = 40;
	constexpr int MIN_NOTES_HEIGHT = 30;
	constexpr int MAX_NOTES_HEIGHT = 60;
	constexpr float MIN_BPM = 10;
	constexpr float MAX_BPM = 10000;
	constexpr int MIN_TIME_SIGN = 1;
	constexpr int MAX_TIME_SIGN = 64;

	constexpr const char* NOTES_TEX = "tex_notes";
	constexpr const char* HOLD_PATH_TEX = "tex_hold_path";
	constexpr const char* HOLD_PATH_CRTCL_TEX = "tex_hold_path_crtcl";
	constexpr const char* HIGHLIGHT_TEX = "tex_note_common_all";
	constexpr const char* APP_CONFIG_FILENAME = "app_config.json";
	constexpr const char* IMGUI_CONFIG_FILENAME = "imgui_config.ini";

	constexpr const char* SE_PERFECT = "perfect";
	constexpr const char* SE_GREAT = "great";
	constexpr const char* SE_GOOD = "good";
	constexpr const char* SE_FLICK = "flick";
	constexpr const char* SE_CONNECT = "connect";
	constexpr const char* SE_TICK = "tick";
	constexpr const char* SE_CRITICAL_TAP = "critical_tap";
	constexpr const char* SE_CRITICAL_FLICK = "critical_flick";
	constexpr const char* SE_CRITICAL_CONNECT = "critical_connect";
	constexpr const char* SE_CRITICAL_TICK = "critical_tick";

	constexpr const char* SE_NAMES[] = {
		SE_PERFECT,
		SE_GREAT,
		SE_GOOD,
		SE_FLICK,
		SE_CONNECT,
		SE_TICK,
		SE_CRITICAL_TAP,
		SE_CRITICAL_FLICK,
		SE_CRITICAL_CONNECT,
		SE_CRITICAL_TICK
	};

	constexpr const char* SUS_EXTENSION = ".sus";
	constexpr const char* MMWS_EXTENSION = ".mmws";
}