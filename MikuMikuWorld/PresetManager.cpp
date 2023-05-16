#include "PresetManager.h"
#include "IO.h"
#include "Application.h"
#include "File.h"
#include "Utilities.h"
#include <fstream>
#include <filesystem>
#include <execution>
#include "JsonIO.h"

#undef min;
#undef max;

using nlohmann::json;

namespace MikuMikuWorld
{
	void PresetManager::loadPresets(const std::string& path)
	{
		std::wstring wPath = IO::mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			return;

		std::vector<std::string> filenames;
		for (const auto& file : std::filesystem::directory_iterator(wPath))
		{
			// look only for json files and ignore any dot files present
			std::wstring wFilename = file.path().filename().wstring();
			if (file.path().extension().wstring() == L".json" && wFilename[0] != L'.')
				filenames.push_back(IO::wideStringToMb(file.path().wstring()));
		}

		std::mutex m2;
		presets.reserve(filenames.size());

		std::vector<Result> warnings;
		std::vector<Result> errors;

		std::for_each(std::execution::par, filenames.begin(), filenames.end(), [this, &warnings, &errors, &m2](const auto& filename) {
			int id = nextPresetID++;

			NotesPreset preset(id, "");
			Result result = preset.read(filename);
			{
				std::lock_guard<std::mutex> lock{ m2 };
				if (result.getStatus() == ResultStatus::Success)
					presets.emplace(id, std::move(preset));

				if (result.getStatus() == ResultStatus::Warning)
					warnings.push_back(result);
				else if (result.getStatus() == ResultStatus::Error)
					errors.push_back(result);
			}
		});

		if (errors.size())
		{
			std::string message;
			for (auto& error : errors)
				message += "- " + error.getMessage() + "\n";

			IO::messageBox(APP_NAME, message, IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error);
		}

		if (warnings.size())
		{
			std::string message;
			for (auto& warning : warnings)
				message += "- " + warning.getMessage() + "\n";

			IO::messageBox(APP_NAME, message, IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Warning);
		}
	}

	void PresetManager::savePresets(const std::string& path)
	{
		std::wstring wPath = IO::mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			std::filesystem::create_directory(wPath);

		for (const std::string& filename : deletePresets)
		{
			std::wstring wFullPath = IO::mbToWideStr(path + filename) + L".json";
			if (std::filesystem::exists(wFullPath))
				std::filesystem::remove(wFullPath);
		}

		std::for_each(std::execution::par, createPresets.begin(), createPresets.end(), [this, &path](int id) {
			if (presets.find(id) != presets.end())
			{
				NotesPreset& preset = presets.at(id);
				std::string filename = path + fixFilename(preset.getName()) + ".json";
				preset.write(filename, false);
			}
		});
	}

	void PresetManager::createPreset(const Score& score, const std::unordered_set<int>& selectedNotes,
		const std::string &name, const std::string& desc)
	{
		if (!selectedNotes.size() || !name.size())
			return;

		NotesPreset preset(nextPresetID++, name);
		preset.name = name;
		preset.description = desc;

		int baseTick = getBaseTick(score, selectedNotes);
		preset.data = jsonIO::noteSelectionToJson(score, selectedNotes, baseTick);

		presets[preset.getID()] = preset;
		createPresets.insert(preset.getID());
	}

	void PresetManager::removePreset(int id)
	{
		if (presets.find(id) == presets.end())
			return;

		const auto& preset = presets.at(id);
		if (preset.getFilename().size())
			deletePresets.insert(preset.getFilename());

		presets.erase(id);
	}

	std::string PresetManager::fixFilename(const std::string& name)
	{
		std::string result = name;
		int length = strlen(invalidFilenameChars);
		for (auto& c : result)
		{
			for (int i = 0; i < length; ++i)
			{
				if (c == invalidFilenameChars[i])
				{
					c = '_';
					break;
				}
			}
		}

		return result;
	}

	int PresetManager::getBaseTick(const Score& score, const std::unordered_set<int>& selection)
	{
		int minTick = INT_MAX;
		for (const int id : selection)
		{
			auto it = score.notes.find(id);
			if (it == score.notes.end())
				continue;

			minTick = std::min(minTick, it->second.tick);
		}

		return minTick;
	}

	void PresetManager::applyPreset(int presetId, ScoreContext& context)
	{
		if (presets.find(presetId) == presets.end())
			return;

		const json data = presets.at(presetId).data;
		if (jsonIO::arrayHasData(data, "notes") || jsonIO::arrayHasData(data, "holds"))
			context.doPasteData(data, false);
	}
}