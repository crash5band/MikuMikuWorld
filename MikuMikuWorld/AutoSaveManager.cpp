#include "AutoSaveManager.h"
#include "Application.h"
#include "StringOperations.h"
#include "Utilities.h"
#include "UI.h"
#include "Localization.h"
#include <filesystem>

namespace MikuMikuWorld
{
	AutoSaveManager::AutoSaveManager()
		: editorInstance{ nullptr }, autoSavePath{ }, enabled{ true }, interval{ 5 }, fileLimit{ 100 }
	{
	}

	void AutoSaveManager::setEditorInstance(ScoreEditor* instance)
	{
		editorInstance = instance;
		autoSavePath = Application::getAppDir() + "auto_save/";
		timer.reset();
	}

	void AutoSaveManager::settings()
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

		UI::beginPropertyColumns();
		UI::addCheckboxProperty(getString("auto_save_enable"), enabled);
		UI::addIntProperty(getString("auto_save_interval"), interval, 1, 60);
		UI::addIntProperty(getString("auto_save_count"), fileLimit, 1, 100);
		UI::endPropertyColumns();

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
	}

	void AutoSaveManager::update()
	{
		if (editorInstance && enabled && timer.elapsedMinutes() >= interval)
		{
			save();
			timer.reset();
		}
	}

	void AutoSaveManager::save()
	{
		std::wstring wAutoSaveDir = mbToWideStr(autoSavePath);

		// create auto save directory if none exists
		if (!std::filesystem::exists(wAutoSaveDir))
			std::filesystem::create_directory(wAutoSaveDir);

		editorInstance->save(autoSavePath + "mmw_auto_save_" + Utilities::getCurrentDateTime() + MMWS_EXTENSION);

		// get mmws files
		int mmwsCount = 0;
		for (const auto& file : std::filesystem::directory_iterator(wAutoSaveDir))
		{
			std::string extension = file.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			mmwsCount += extension == MMWS_EXTENSION;
		}

		// delete older files
		if (mmwsCount > fileLimit)
			deleteOldAutoSave(mmwsCount - fileLimit);
	}

	int AutoSaveManager::deleteOldAutoSave(int count)
	{
		std::wstring wAutoSaveDir = mbToWideStr(autoSavePath);
		if (!std::filesystem::exists(wAutoSaveDir))
			return 0;

		// get mmws files
		using entry = std::filesystem::directory_entry;
		std::vector<entry> deleteFiles;
		for (const auto& file : std::filesystem::directory_iterator(wAutoSaveDir))
		{
			std::string extension = file.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			if (extension == MMWS_EXTENSION)
				deleteFiles.push_back(file);
		}

		// sort files by modification date
		std::sort(deleteFiles.begin(), deleteFiles.end(), [](const entry& f1, const entry& f2) {
			return f1.last_write_time() < f2.last_write_time();
		});

		int deleteCount = 0;
		while (count && deleteFiles.size())
		{
			std::filesystem::remove(deleteFiles.begin()->path());
			deleteFiles.erase(deleteFiles.begin());

			--count;
			++deleteCount;
		}

		return deleteCount;
	}

	void AutoSaveManager::readConfig(const ApplicationConfiguration& config)
	{
		enabled = config.autoSaveEnabled;
		interval = config.autoSaveInterval;
		fileLimit = config.autoSaveMaxCount;
	}

	void AutoSaveManager::writeConfig(ApplicationConfiguration& config)
	{
		config.autoSaveEnabled = enabled;
		config.autoSaveInterval = interval;
		config.autoSaveMaxCount = fileLimit;
	}
}