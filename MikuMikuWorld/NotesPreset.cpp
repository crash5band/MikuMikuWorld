#include "NotesPreset.h"
#include "IO.h"
#include "Application.h"
#include "File.h"
#include "Utilities.h"
#include <fstream>
#include <filesystem>
#include <execution>
#include "JsonIO.h"

using nlohmann::json;

namespace MikuMikuWorld
{
	namespace fs = std::filesystem;
	
	NotesPreset::NotesPreset(int _id, std::string _name) :
		ID{ _id }, name{ _name }
	{
	}

	NotesPreset::NotesPreset() : ID{ -1 }, name{ "" }, description{ "" }
	{
	}

	bool isValidHoldJson(const json& hold)
	{
		bool anyEndMissing =  !hold.contains("start") || !hold.contains("end");
		if (anyEndMissing)
			return false;

		int startTick = jsonIO::tryGetValue<int>(hold["start"], "tick", 0);
		int endTick = jsonIO::tryGetValue<int>(hold["end"], "tick", 0);
		if (endTick <= startTick)
			return false;

		if (hold.contains("steps"))
		{
			const json& stepData = hold["steps"];
			bool anyStepHasInvalidTick = std::any_of(stepData.cbegin(), stepData.cend(), [startTick](const json& step)
			{
				int stepTick = jsonIO::tryGetValue<int>(step, "tick", 0);
				return stepTick <= startTick;
			});

			return !anyStepHasInvalidTick;
		}

		return true;
	}

	Result NotesPreset::read(const std::string& filepath)
	{
		std::wstring wFilename = IO::mbToWideStr(filepath);
		if (!std::filesystem::exists(wFilename))
			return Result(ResultStatus::Error, "The preset file " + filepath + " does not exist.");

		std::ifstream file(wFilename);
		file >> data;
		file.close();

		filename = IO::File::getFilenameWithoutExtension(filepath);
		name = jsonIO::tryGetValue<std::string>(data, "name", "");

		if (data.find("description") != data.end())
			description = data["description"];

		if (data.find("notes") == data.end() && data.find("holds") == data.end())
			return Result(ResultStatus::Warning, "The preset " + filename + " does not contain any notes data. Skipping...");

		if (data.find("holds") != data.end())
		{
			const json& holdData = data["holds"];
			bool hasAnyInvalidHold = std::any_of(holdData.cbegin(), holdData.cend(), std::not_fn(isValidHoldJson));
			if (hasAnyInvalidHold)
				return Result(ResultStatus::Error, "The preset " + filename + " contains invalid hold data. Skipping...");
		}

		if (name.empty())
		{
			name = filename;
			return Result(ResultStatus::Warning, "The preset " + filename + " does not have a name. Using filename instead.");
		}
		
		return Result::Ok();
	}

	void NotesPreset::write(std::string filepath, bool overwrite)
	{
		std::wstring wFilename = IO::mbToWideStr(filepath);
		if (!overwrite)
		{
			int count = 1;
			std::wstring suffix = L".json";

			while (std::filesystem::exists(wFilename + suffix))
				suffix = L"(" + std::to_wstring(count++) + L").json";

			wFilename += suffix;
		}

		data["name"] = name;
		data["description"] = description;

		std::ofstream file(wFilename);

		file << std::setw(2) << data;
		file.flush();
		file.close();
	}

	void PresetManager::loadPresets(const std::string& path)
	{
		std::wstring wPath = IO::mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			return;

		std::vector<std::string> filenames;
		for (const auto& file : std::filesystem::directory_iterator(wPath))
		{
			// Ignore dot files
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
				{
					presets.emplace(id, std::move(preset));
				}
				else if (result.getStatus() == ResultStatus::Warning)
				{
					presets.emplace(id, std::move(preset));
					warnings.push_back(result);
				}
				else if (result.getStatus() == ResultStatus::Error)
				{
					errors.push_back(result);
				}
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
		namespace fs = std::filesystem;
		fs::path libPath{ path };

		std::wstring wPath = IO::mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			std::filesystem::create_directory(wPath);

		for (const std::string& filename : deletePresets)
		{
			std::wstring wFullPath = IO::mbToWideStr((libPath / filename).string()) + L".json";
			if (std::filesystem::exists(wFullPath))
				std::filesystem::remove(wFullPath);
		}

		std::for_each(std::execution::par, createPresets.begin(), createPresets.end(), [this, &libPath](int id) {
			if (presets.find(id) != presets.end())
			{
				NotesPreset& preset = presets.at(id);

				// filename without extension
				// we will add the extension later after determining what the final filename should be
				std::string filename = (libPath / fixFilename(preset.getName())).string();
				preset.write(filename, false);
			}
		});
	}

	void PresetManager::createPreset(const ScoreContext& context, const std::string &name, const std::string& desc)
	{
		if (context.selectedNotes.empty() || name.empty())
			return;

		NotesPreset preset(nextPresetID++, name);
		preset.name = name;
		preset.description = desc;
		preset.data = jsonIO::noteSelectionToJson(context.score, context.selectedNotes, context.minTickFromSelection());

		presets[preset.getID()] = preset;
		createPresets.push_back(preset.getID());
	}

	void PresetManager::removePreset(int id)
	{
		if (presets.find(id) == presets.end())
			return;

		const auto& preset = presets.at(id);
		if (preset.getFilename().size())
			deletePresets.push_back(preset.getFilename());

		presets.erase(id);
	}

	std::string PresetManager::fixFilename(const std::string& name)
	{
		std::string result = name;
		constexpr std::array<char, 9> invalidFilenameChars{ '\\', '/', '\"', '|', '<', '>', '?', '*', ':' };
		for (char c : invalidFilenameChars)
		{
			std::replace(result.begin(), result.end(), c, '_');
		}

		return result;
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