#include "NotesPreset.h"
#include "IO.h"
#include "Application.h"
#include "File.h"
#include "Utilities.h"
#include <fstream>
#include <filesystem>
#include <execution>
#include "JsonIO.h"

using namespace nlohmann;
namespace fs = std::filesystem;

namespace MikuMikuWorld
{
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
		fs::path path{IO::mbToWideStr(filepath)};
		if (!fs::exists(path))
			return Result(ResultStatus::Error, "The preset file \"" + filepath + "\" does not exist.");

		std::ifstream file(path);
		file >> data;
		file.close();

		filename = IO::File::getFilenameWithoutExtension(filepath);
		name = jsonIO::tryGetValue<std::string>(data, "name", "");

		if (data.contains("description"))
			description = data["description"];

		if (!data.contains("notes") && !data.contains("holds"))
			return Result(ResultStatus::Warning, "The preset \"" + filename + "\" does not contain any notes data. Skipping...");

		if (data.contains("holds"))
		{
			const json& holdData = data["holds"];
			bool hasAnyInvalidHold = std::any_of(holdData.cbegin(), holdData.cend(), std::not_fn(isValidHoldJson));
			if (hasAnyInvalidHold)
				return Result(ResultStatus::Error, "The preset \"" + filename + "\" contains invalid hold data. Skipping...");
		}

		if (name.empty())
		{
			name = filename;
			return Result(ResultStatus::Warning, "The preset \"" + filename + "\" does not have a name. Using filename instead.");
		}
		
		return Result::Ok();
	}

	void NotesPreset::write(std::string filepath, bool overwrite)
	{
		std::wstring wFilename = IO::mbToWideStr(filepath);
		if (!overwrite)
		{
			int count = 1;
			std::wstring suffix = L"";

			while (fs::exists(wFilename + suffix))
				suffix = L"(" + std::to_wstring(count++) + L")";

			wFilename += suffix;
		}

		data["name"] = name;
		data["description"] = description;
		
		std::ofstream file(fs::path{wFilename}.replace_extension(".json"));

		file << std::setw(2) << data;
		file.flush();
		file.close();

		filename = IO::File::getFilenameWithoutExtension(IO::wideStringToMb(wFilename));
	}

	PresetManager::PresetManager(const std::string& path) : presetsPath{ IO::mbToWideStr(path) }
	{
		fs::create_directory(presetsPath);
	}

	void PresetManager::loadPresets()
	{
		if (!fs::exists(presetsPath))
			return;

		std::vector<std::string> filenames;
		for (const auto& file : std::filesystem::directory_iterator(presetsPath))
		{
			// Ignore dot files
			if (file.path().extension().wstring() == L".json" && file.path().wstring().at(0) != L'.')
				filenames.push_back(IO::wideStringToMb(file.path().wstring()));
		}

		std::mutex m2;
		presets.reserve(filenames.size());

		std::vector<Result> warnings;
		std::vector<Result> errors;

		std::for_each(std::execution::par, filenames.begin(), filenames.end(),
			[this, &warnings, &errors, &m2](const auto& filename) {

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

			IO::messageBox(APP_NAME, message, IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Error, Application::windowState.windowHandle);
		}

		if (warnings.size())
		{
			std::string message;
			for (auto& warning : warnings)
				message += "- " + warning.getMessage() + "\n";

			IO::messageBox(APP_NAME, message, IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Warning, Application::windowState.windowHandle);
		}
	}

	Result PresetManager::importPreset(const std::string& path)
	{
		fs::path importPath{IO::mbToWideStr(path)};
		if (!fs::exists(importPath))
			return Result(ResultStatus::Error, "File does not exist");
		
		int id = nextPresetID++;
		NotesPreset preset = NotesPreset(id, "");
		Result result = preset.read(path);

		if (result.getStatus() == ResultStatus::Error)
			return result;
		
		fs::path dir{IO::File::getFilepath(path)};
		if (dir != presetsPath)
			fs::copy_file(importPath, (presetsPath / IO::File::getFilenameWithoutExtension(path)).replace_extension(".json"));
		
		presets.insert(std::pair<int, NotesPreset>(id, std::move(preset)));
		return result;
	}

	void PresetManager::createPreset(const ScoreContext& context, const std::string &name, const std::string& desc)
	{
		if (context.selectedNotes.empty() || name.empty())
			return;

		NotesPreset preset(nextPresetID++, name);
		preset.description = desc;
		preset.data = jsonIO::noteSelectionToJson(context.score, context.selectedNotes, context.minTickFromSelection());

		presets[preset.getID()] = preset;
		if (createPresetFuture.valid())
			createPresetFuture.get();

		createPresetFuture = std::async(std::launch::async, &PresetManager::savePreset, this, preset);
	}

	void PresetManager::savePreset(NotesPreset preset)
	{
		fs::create_directory(presetsPath);
		preset.write((presetsPath / fixFilename(preset.getName())).string(), false);
		presets.insert_or_assign(preset.getID(), std::move(preset));
	}

	void PresetManager::removePreset(int id)
	{
		if (presets.find(id) == presets.end())
			return;

		const auto& preset = presets[id];
		if (preset.getFilename().empty())
			return;
		
		if (createPresetFuture.valid())
			createPresetFuture.get();

		if (deletePresetFuture.valid())
			deletePresetFuture.get();

		deletePresetFuture = std::async(std::launch::async, [this, preset]() -> bool
		{
			deletedPreset = std::move(preset);
			return fs::remove((presetsPath / deletedPreset.getFilename()).replace_extension(".json"));
		});

		presets.erase(id);
	}

	bool PresetManager::undoDeletePreset()
	{
		deletedPreset.write(IO::wideStringToMb((presetsPath / deletedPreset.getFilename()).wstring()), false);
		presets.insert_or_assign(deletedPreset.getID(), std::move(deletedPreset));
		deletedPreset = {};
		return true;
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

		const json& data = presets.at(presetId).data;
		if (jsonIO::arrayHasData(data, "notes") || jsonIO::arrayHasData(data, "holds"))
			context.doPasteData(data, false);
	}
}