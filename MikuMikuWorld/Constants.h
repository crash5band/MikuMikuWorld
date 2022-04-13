#pragma once

namespace MikuMikuWorld
{
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
	constexpr int MIN_DIFFICULTY = 1;
	constexpr int MAX_DIFFICULTY = 40;

	constexpr const char* NOTES_TEX = "tex_notes";
	constexpr const char* HOLD_PATH_TEX = "tex_hold_path";
	constexpr const char* HOLD_PATH_CRTCL_TEX = "tex_hold_path_crtcl";
	constexpr const char* HIGHLIGHT_TEX = "tex_note_common_all";

	constexpr int TICKS_PER_BEAT = 480;
}