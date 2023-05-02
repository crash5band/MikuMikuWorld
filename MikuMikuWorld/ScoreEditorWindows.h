#pragma once
#include "PresetManager.h"
#include "ScoreEditorTimeline.h"

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
	public:
		bool open = false;
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
		inline void close() { ImGui::CloseCurrentPopup(); open = false; }

		DialogResult update();
	};
}