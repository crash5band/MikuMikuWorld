#pragma once
#include <string>
#include "Stopwatch.h"

namespace MikuMikuWorld
{
	class ScoreEditor;
	class ApplicationConfiguration;

	class AutoSaveManager
	{
	private:
		ScoreEditor* editorInstance;
		Stopwatch timer;

		std::string autoSavePath;
		bool enabled;
		int interval; // in minutes
		int fileLimit;

	public:
		AutoSaveManager();

		void setEditorInstance(ScoreEditor* instance);
		void settings();
		void readConfig(const ApplicationConfiguration& config);
		void writeConfig(ApplicationConfiguration& config);

		void update();
		void save();
		int deleteOldAutoSave(int count);
	};
}