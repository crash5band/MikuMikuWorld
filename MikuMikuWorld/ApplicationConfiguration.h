#pragma once
#include "UI.h"
#include "json.hpp"
#include "Math.h"
#include "InputBinding.h"

namespace MikuMikuWorld
{
	struct InputConfiguration
	{
		MultiInputBinding togglePlayback = { "toggle_playback", InputBinding{ImGuiKey_Space, ImGuiKeyModFlags_None} };
		MultiInputBinding deleteSelection = { "delete", InputBinding{ImGuiKey_Delete} };
		MultiInputBinding copySelection = { "copy", InputBinding{ImGuiKey_C, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding cutSelection = { "cut", InputBinding{ImGuiKey_X, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding paste = { "paste", InputBinding{ImGuiKey_V, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding flipPaste = { "flip_paste", InputBinding{ImGuiKey_V, ImGuiKeyModFlags_Ctrl | ImGuiKeyModFlags_Shift} };
		MultiInputBinding flip = { "flip", InputBinding{ImGuiKey_F, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding previousTick = { "previous_tick", InputBinding{ImGuiKey_DownArrow} };
		MultiInputBinding nextTick = { "next_tick", InputBinding{ImGuiKey_UpArrow} };
		MultiInputBinding create = { "new_chart", InputBinding{ImGuiKey_N, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding open = { "open", InputBinding{ImGuiKey_O, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding save = { "save", InputBinding{ImGuiKey_S, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding saveAs = { "save_as", InputBinding{ImGuiKey_S, ImGuiKeyModFlags_Ctrl | ImGuiKeyModFlags_Shift} };
		MultiInputBinding exportSus = { "export", InputBinding{ImGuiKey_E, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding selectAll = { "select_all", InputBinding{ImGuiKey_A, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding undo = { "undo", InputBinding{ImGuiKey_Z, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding redo = { "redo", InputBinding{ImGuiKey_Y, ImGuiKeyModFlags_Ctrl} };
		
		MultiInputBinding timelineSelect = { "select", InputBinding{ImGuiKey_1} };
		MultiInputBinding timelineTap = { "tap", InputBinding{ImGuiKey_2} };
		MultiInputBinding timelineHold = { "hold", InputBinding{ImGuiKey_3} };
		MultiInputBinding timelineHoldMid = { "hold_mid", InputBinding{ImGuiKey_4} };
		MultiInputBinding timelineFlick = { "flick", InputBinding{ImGuiKey_5} };
		MultiInputBinding timelineCritical = { "critical", InputBinding{ImGuiKey_6} };
		MultiInputBinding timelineBpm = { "bpm", InputBinding{ImGuiKey_7} };
		MultiInputBinding timelineTimeSignature = { "time_signature", InputBinding{ImGuiKey_8} };
		MultiInputBinding timelineHiSpeed = { "hi_speed", InputBinding{ImGuiKey_9} };
	};

	struct ApplicationConfiguration
	{
		std::string version;

		// session state
		Vector2 windowPos;
		Vector2 windowSize;
		bool maximized;
		bool vsync;
		int accentColor;
		Color userColor;
		BaseTheme baseTheme;
		int division;
		float zoom;

		// settings
		int timelineWidth;
		int notesHeight;
		float laneOpacity;
		float backgroundBrightness;
		bool useSmoothScrolling;
		float smoothScrollingTime;
		float cursorPositionThreshold;
		bool returnToLastSelectedTickOnPause;
		bool followCursorInPlayback;
		bool autoSaveEnabled;
		int autoSaveInterval;
		int autoSaveMaxCount;
		float masterVolume;
		float bgmVolume;
		float seVolume;

		InputConfiguration input;

		ApplicationConfiguration();

		void read(const std::string& filename);
		void write(const std::string& filename);
		void restoreDefault();
	};
	
	extern ApplicationConfiguration config;

	static MultiInputBinding* bindings[] =
	{
		&config.input.create,
		&config.input.open,
		&config.input.save,
		&config.input.saveAs,
		&config.input.exportSus,
		&config.input.cutSelection,
		&config.input.copySelection,
		&config.input.paste,
		&config.input.flipPaste,
		&config.input.deleteSelection,
		&config.input.previousTick,
		&config.input.nextTick,
		&config.input.selectAll,
		&config.input.undo,
		&config.input.redo,
		&config.input.timelineSelect,
		&config.input.timelineTap,
		&config.input.timelineHold,
		&config.input.timelineHoldMid,
		&config.input.timelineFlick,
		&config.input.timelineCritical,
		&config.input.timelineBpm,
		&config.input.timelineTimeSignature,
		&config.input.timelineHiSpeed
	};
}