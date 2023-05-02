#include "ScoreEditorWindows.h"

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

		std::string exportComment;

		bool save(std::string filename);

	public:
		ScoreEditor();

		void update();

		void create();
		void open();
		void loadScore(std::string filename);
		void exportSus();
		bool saveAs();
		bool trySave(std::string filename = "");

		void drawMenubar();
		void drawToolbar();

		inline void loadPresets(std::string path) { presetManager.loadPresets(path); }
		inline void savePresets(std::string path) { presetManager.savePresets(path); }
		inline void loadMusic(std::string path) { context.audio.changeBGM(path); }

		inline void uninitialize() { context.audio.uninitAudio(); }
		inline const char* getWorkingFilename() const { return context.workingData.filename.c_str(); }
		constexpr inline bool isUpToDate() const { return context.upToDate; }
	};
}