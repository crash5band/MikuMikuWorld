#pragma once
#include "ScoreEditorWindows.h"
#include <future>

namespace MikuMikuWorld
{
	class ScoreEditor
	{
	private:
		ScoreContext context{};
		EditArgs edit{};
		std::unique_ptr<Renderer> renderer;
		PresetManager presetManager;

		ScoreEditorTimeline timeline{};
		ScorePropertiesWindow propertiesWindow{};
		ScoreOptionsWindow optionsWindow{};
		PresetsWindow presetsWindow{};
		DebugWindow debugWindow{};
		SettingsWindow settingsWindow{};
		RecentFileNotFoundDialog recentFileNotFoundDialog{};
		AboutDialog aboutDialog{};

		Stopwatch autoSaveTimer;
		std::string autoSavePath;
		bool showImGuiDemoWindow{false};

		std::future<void> loadScoreFuture{};
		std::future<void> loadMusicFuture{};
		std::future<void> loadPresetsFuture{};
		std::future<void> importPresetFuture{};

		bool save(std::string filename);
		size_t updateRecentFilesList(const std::string& entry);

	public:
		ScoreEditor();

		void update();

		void create();
		void open();
		void loadScore(std::string filename);
		void asyncLoadScore(std::string filename);
		void loadMusic(std::string filename);
		void asyncLoadMusic(std::string filename);
		void exportSus();
		bool saveAs();
		bool trySave(std::string);
		void autoSave();
		int deleteOldAutoSave(int count);

		void drawMenubar();
		void drawToolbar();
		void help();

		void loadPresets();
		void openImportPresetDialog();
		void importPreset(const std::string& path);
		void writeSettings();
		void uninitialize();
		inline std::string_view getWorkingFilename() const { return context.workingData.filename; }
		constexpr inline bool isUpToDate() const { return context.upToDate; }
	};
}