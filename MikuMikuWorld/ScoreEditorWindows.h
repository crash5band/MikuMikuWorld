#pragma once
#include "PresetManager.h"
#include "ScoreEditorTimeline.h"
#include "Stopwatch.h"
#include "InputBinding.h"

namespace MikuMikuWorld
{
	enum class DialogResult
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
		void update(ScoreContext& context);
	};

	class ScoreOptionsWindow
	{
	public:
		void update(ScoreContext& context, EditArgs& edit, TimelineMode currentMode);
	};

	class PresetsWindow
	{
	private:
		ImGuiTextFilter presetFilter;
		std::string presetName{};
		std::string presetDesc{};
		bool dialogOpen = false;

		DialogResult updateCreationDialog();

	public:
		void update(ScoreContext& context, PresetManager& presetManager);
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

	public:
		bool open = false;
		bool isBackgroundChangePending = false;
		DialogResult update();
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