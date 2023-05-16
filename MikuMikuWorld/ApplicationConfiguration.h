#pragma once
#include "UI.h"
#include "json.hpp"
#include "Math.h"
#include "InputBinding.h"

namespace MikuMikuWorld
{
	struct InputConfiguration
	{
		MultiInputBinding togglePlayback = { "toggle_playback", {ImGuiKey_Space, ImGuiKeyModFlags_None} };
		MultiInputBinding stop = { "stop", {} };
		MultiInputBinding decreaseNoteSize = { "decrease_note_size", {} };
		MultiInputBinding increaseNoteSize = { "decrease_note_size", {} };
		MultiInputBinding shrinkDown = { "shrink_down", {} };
		MultiInputBinding shrinkUp = { "shrink_up", {} };
		MultiInputBinding openHelp = { "help", {} };
		MultiInputBinding openSettings = { "settings", {} };
		MultiInputBinding deleteSelection = { "delete", {ImGuiKey_Delete} };
		MultiInputBinding copySelection = { "copy", {ImGuiKey_C, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding cutSelection = { "cut", {ImGuiKey_X, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding paste = { "paste", {ImGuiKey_V, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding flipPaste = { "flip_paste", {ImGuiKey_V, ImGuiKeyModFlags_Ctrl | ImGuiKeyModFlags_Shift} };
		MultiInputBinding flip = { "flip", {ImGuiKey_F, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding cancelPaste = { "cancel_paste", {ImGuiKey_Escape} };
		MultiInputBinding previousTick = { "previous_tick", {ImGuiKey_DownArrow} };
		MultiInputBinding nextTick = { "next_tick", {ImGuiKey_UpArrow} };
		MultiInputBinding create = { "new_chart", {ImGuiKey_N, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding open = { "open", {ImGuiKey_O, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding save = { "save", {ImGuiKey_S, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding saveAs = { "save_as", {ImGuiKey_S, ImGuiKeyModFlags_Ctrl | ImGuiKeyModFlags_Shift} };
		MultiInputBinding exportSus = { "export", {ImGuiKey_E, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding selectAll = { "select_all", {ImGuiKey_A, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding undo = { "undo", {ImGuiKey_Z, ImGuiKeyModFlags_Ctrl} };
		MultiInputBinding redo = { "redo", {ImGuiKey_Y, ImGuiKeyModFlags_Ctrl} };
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