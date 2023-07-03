#pragma once
#include "UI.h"
#include "json.hpp"
#include "Math.h"
#include "InputBinding.h"

namespace MikuMikuWorld
{
	struct InputConfiguration
	{
		MultiInputBinding togglePlayback = { "toggle_playback", {ImGuiKey_Space, ImGuiModFlags_None} };
		MultiInputBinding stop = { "stop", {} };
		MultiInputBinding decreaseNoteSize = { "decrease_note_size", {} };
		MultiInputBinding increaseNoteSize = { "increase_note_size", {} };
		MultiInputBinding shrinkDown = { "shrink_down", {} };
		MultiInputBinding shrinkUp = { "shrink_up", {} };
		MultiInputBinding openHelp = { "help", {ImGuiKey_F1} };
		MultiInputBinding openSettings = { "settings", {} };
		MultiInputBinding deleteSelection = { "delete", {ImGuiKey_Delete} };
		MultiInputBinding copySelection = { "copy", {ImGuiKey_C, ImGuiModFlags_Ctrl} };
		MultiInputBinding cutSelection = { "cut", {ImGuiKey_X, ImGuiModFlags_Ctrl} };
		MultiInputBinding paste = { "paste", {ImGuiKey_V, ImGuiModFlags_Ctrl} };
		MultiInputBinding flipPaste = { "flip_paste", {ImGuiKey_V, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift} };
		MultiInputBinding flip = { "flip", {ImGuiKey_F, ImGuiModFlags_Ctrl} };
		MultiInputBinding cancelPaste = { "cancel_paste", {ImGuiKey_Escape} };
		MultiInputBinding previousTick = { "previous_tick", {ImGuiKey_DownArrow} };
		MultiInputBinding nextTick = { "next_tick", {ImGuiKey_UpArrow} };
		MultiInputBinding create = { "new_chart", {ImGuiKey_N, ImGuiModFlags_Ctrl} };
		MultiInputBinding open = { "open", {ImGuiKey_O, ImGuiModFlags_Ctrl} };
		MultiInputBinding save = { "save", {ImGuiKey_S, ImGuiModFlags_Ctrl} };
		MultiInputBinding saveAs = { "save_as", {ImGuiKey_S, ImGuiModFlags_Ctrl | ImGuiModFlags_Shift} };
		MultiInputBinding exportSus = { "export", {ImGuiKey_E, ImGuiModFlags_Ctrl} };
		MultiInputBinding selectAll = { "select_all", {ImGuiKey_A, ImGuiModFlags_Ctrl} };
		MultiInputBinding undo = { "undo", {ImGuiKey_Z, ImGuiModFlags_Ctrl} };
		MultiInputBinding redo = { "redo", {ImGuiKey_Y, ImGuiModFlags_Ctrl} };
		MultiInputBinding zoomOut = { "zoom_out", {} };
		MultiInputBinding zoomIn = { "zoom_in", {} };
		
		MultiInputBinding timelineSelect = { "timeline_select", {ImGuiKey_1} };
		MultiInputBinding timelineTap = { "timeline_tap", {ImGuiKey_2} };
		MultiInputBinding timelineHold = { "timeline_hold", {ImGuiKey_3} };
		MultiInputBinding timelineHoldMid = { "timeline_hold_step", {ImGuiKey_4} };
		MultiInputBinding timelineFlick = { "timeline_flick", {ImGuiKey_5} };
		MultiInputBinding timelineCritical = { "timeline_critical", {ImGuiKey_6} };
		MultiInputBinding timelineBpm = { "timeline_bpm", {ImGuiKey_7} };
		MultiInputBinding timelineTimeSignature = { "timeline_time_signature", {ImGuiKey_8} };
		MultiInputBinding timelineHiSpeed = { "timeline_hi_speed", {ImGuiKey_9} };
	};

	struct ApplicationConfiguration
	{
		std::string version;

		// session state
		Vector2 windowPos;
		Vector2 windowSize;
		bool maximized;
		bool vsync;
		bool showFPS;
		int accentColor;
		Color userColor;
		BaseTheme baseTheme;
		std::string language;
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
		bool debugEnabled;

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
		&config.input.cancelPaste,
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
		&config.input.timelineHiSpeed,
		&config.input.zoomOut,
		&config.input.zoomIn,
		&config.input.openHelp,
		&config.input.openSettings,
		&config.input.decreaseNoteSize,
		&config.input.increaseNoteSize,
		&config.input.shrinkDown,
		&config.input.shrinkUp,
	};
}