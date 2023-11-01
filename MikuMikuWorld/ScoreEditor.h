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
		SettingsWindow settingsWindow{};
		RecentFileNotFoundDialog recentFileNotFoundDialog{};
		AboutDialog aboutDialog{};

		Stopwatch autoSaveTimer;
		std::string autoSavePath;
		bool showImGuiDemoWindow;

		bool save(std::string filename);
		size_t updateRecentFilesList(const std::string& entry);

	public:
		ScoreEditor();

		void update();

		void create();
		void open();
		void loadScore(std::string filename);
		void loadMusic(std::string filename);
		void exportSus();
		bool saveAs();
		bool trySave(std::string);
		void autoSave();
		int deleteOldAutoSave(int count);

		void drawMenubar();
		void drawToolbar();
		void help();

		inline void loadPresets(std::string path) { presetManager.loadPresets(path); }
		inline void savePresets(std::string path) { presetManager.savePresets(path); }

		void writeSettings();
		void uninitialize();
		inline std::string_view getWorkingFilename() const { return context.workingData.filename; }
		constexpr inline bool isUpToDate() const { return context.upToDate; }
	};
}