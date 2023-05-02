#include "PresetManager.h"
#include "StringOperations.h"
#include "Application.h"
#include "File.h"
#include "Utilities.h"
#include <tinyfiledialogs.h>
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
		std::wstring wPath = mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			return;

		std::vector<std::string> filenames;
		for (const auto& file : std::filesystem::directory_iterator(wPath))
		{
			// look only for json files and ignore any dot files present
			std::wstring wFilename = file.path().filename().wstring();
			if (file.path().extension().wstring() == L".json" && wFilename[0] != L'.')
				filenames.push_back(wideStringToMb(file.path().wstring()));
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

			tinyfd_messageBox(APP_NAME, message.c_str(), "ok", "error", 1);
		}

		if (warnings.size())
		{
			std::string message;
			for (auto& warning : warnings)
				message += "- " + warning.getMessage() + "\n";

			tinyfd_messageBox(APP_NAME, message.c_str(), "ok", "warning", 1);
		}
	}

	void PresetManager::savePresets(const std::string& path)
	{
		std::wstring wPath = mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			std::filesystem::create_directory(wPath);

		for (const std::string& filename : deletePresets)
		{
			std::wstring wFullPath = mbToWideStr(path + filename) + L".json";
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

		// copy the preset notes, and assign new IDs
		NotesPreset& preset = presets.at(presetId);
		std::unordered_map<int, Note> notes;
		std::unordered_map<int, HoldNote> holdNotes;

		if (jsonIO::keyExists(preset.data, "notes") && !preset.data["notes"].is_null())
		{
			for (const auto& entry : preset.data["notes"])
			{
				Note note = jsonIO::jsonToNote(entry, NoteType::Tap);
				note.tick += context.currentTick;
				note.ID = nextID++;

				notes[note.ID] = note;
			}
		}

		if (jsonIO::keyExists(preset.data, "holds") && !preset.data["holds"].is_null())
		{
			for (const auto& entry : preset.data["holds"])
			{
				Note start = jsonIO::jsonToNote(entry["start"], NoteType::Hold);
				start.tick += context.currentTick;
				start.ID = nextID++;
				notes[start.ID] = start;

				Note end = jsonIO::jsonToNote(entry["end"], NoteType::HoldEnd);
				end.tick += context.currentTick;
				end.ID = nextID++;
				end.parentID = start.ID;
				notes[end.ID] = end;

				std::string startEase = entry["start"]["ease"];
				HoldNote hold;
				hold.start = { start.ID, HoldStepType::Visible, (EaseType)findArrayItem(startEase.c_str(), easeTypes, TXT_ARR_SZ(easeTypes)) };
				hold.end = end.ID;

				if (jsonIO::keyExists(entry, "steps"))
				{
					hold.steps.reserve(entry["steps"].size());
					for (const auto& step : entry["steps"])
					{
						Note mid = jsonIO::jsonToNote(step, NoteType::HoldMid);
						mid.tick += context.currentTick;
						mid.critical = start.critical;
						mid.ID = nextID++;
						mid.parentID = start.ID;
						notes[mid.ID] = mid;

						std::string midType = step["type"];
						std::string midEase = step["ease"];
						hold.steps.push_back(
							{
								mid.ID,
								(HoldStepType)findArrayItem(midType.c_str(), stepTypes, TXT_ARR_SZ(stepTypes)),
								(EaseType)findArrayItem(midEase.c_str(), easeTypes, TXT_ARR_SZ(easeTypes))
							});
					}
				}

				holdNotes[hold.start.ID] = hold;
			}
		}

		context.selectedNotes.clear();
		std::transform(notes.begin(), notes.end(),
			std::inserter(context.selectedNotes, context.selectedNotes.end()),
			[this](const auto& it) { return it.second.ID; });

		context.score.notes.insert(notes.begin(), notes.end());
		context.score.holdNotes.insert(holdNotes.begin(), holdNotes.end());
	}
}