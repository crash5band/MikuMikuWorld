#pragma once
#include "NotesPreset.h"
#include "ScoreEditorTimeline.h"
#include "Stopwatch.h"
#include "InputBinding.h"

namespace MikuMikuWorld
{
	enum class DialogResult : uint8_t
	{
		None,
		Ok,
		Cancel,
		Yes,
		No,
		Retry
	};

	class ScorePropertiesWindow
	{
	public:
		std::string pendingLoadMusicFilename{};
		bool isPendingLoadMusic{ false };
		bool isLoadingMusic{ false };
		bool isLoadingJacket{ false };
		void update(ScoreContext& context);

	private:
		std::string loadingText = "Loading...";
		std::array<uint8_t, 5> scoreStatsImages { 3, 0, 1, 2, 4 };

		void statsTableRow(const char* lbl, size_t row);
	};

	class ScoreOptionsWindow
	{
	public:
		void update(ScoreContext& context, EditArgs& edit, TimelineMode currentMode);
	};

	class PresetsWindow
	{
	private:
		ImGuiTextFilter presetFilter{};
		ImGuiListClipper listClipper{};
		std::string presetName{};
		std::string presetDesc{};
		std::vector<int> filterMatchPresets;
		bool dialogOpen{false};
		bool loadingPresets{ false };

		DialogResult updateCreationDialog();
		void memoizeFilterMatches(const std::vector<NotesPreset>& items);

	public:
		void update(ScoreContext& context, PresetManager& presetManager);
		inline void openCreatePresetDialog() { dialogOpen = true; }

		void notifyPresetsLoading();
		void notifyPresetsLoaded(const PresetManager& presetManager);
	};

	class DebugWindow
	{
	public:
		void update(ScoreContext& context, ScoreEditorTimeline& timeline);
	};

	class SettingsWindow
	{
	private:
		Stopwatch inputTimer;
		const int inputTimeoutSeconds = 5;
		bool listeningForInput = false;
		int editBindingIndex = -1;
		int selectedBindingIndex = 0;

		void updateKeyConfig(MultiInputBinding* bindings[], int count);
		void setEffectsProfile(int profile);

	public:
		bool open = false;
		bool isBackgroundChangePending = false;
		bool isEffectProfileChangePending = false;
		DialogResult update();
	};

	class RecentFileNotFoundDialog
	{
	public:
		std::string removeFilename;
		size_t removeIndex{ 0 };
		bool open{ false };

		DialogResult update();
		inline void close() { ImGui::CloseCurrentPopup(); }
	};

	class UnsavedChangesDialog
	{
	public:
		bool open = false;
		inline void close() { ImGui::CloseCurrentPopup(); open = false; }

		DialogResult update();
	};

	class AboutDialog
	{
	public:
		bool open = false;
		DialogResult update();
	};
}