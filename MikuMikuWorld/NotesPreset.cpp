#include "Application.h"
#include "NotesPreset.h"
#include "IO.h"
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

		filename = IO::File::getFilename(filepath);
		name = jsonIO::tryGetValue<std::string>(data, "name", "");

		if (data.contains("description"))
			description = data["description"];

		if (!data.contains("notes") && !data.contains("holds"))
			return Result(ResultStatus::Error, "The preset \"" + filename + "\" does not contain any notes data. Skipping...");

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

	void NotesPreset::write(fs::path filepath, bool overwrite)
	{
		std::wstring wFilename = IO::File::getFullFilenameWithoutExtension(filepath.wstring());
		if (!overwrite)
		{
			int count = 1;
			std::wstring suffix = L"";

			while (fs::exists(wFilename + suffix + L".json"))
				suffix = L"(" + std::to_wstring(count++) + L")";

			wFilename += suffix;
		}

		data["name"] = name;
		data["description"] = description;
		
		std::ofstream file(fs::path(wFilename).replace_extension(L".json"));
		file.exceptions(std::ios::badbit | std::ios::failbit);
		file << std::setw(2) << data;
		file.flush();
		file.close();

		filename = IO::File::getFilename(IO::wideStringToMb(wFilename));
	}

	IO::MessageBoxResult PresetManager::showErrorMessage(const std::string& message)
	{
		return IO::messageBox(
			APP_NAME,
			message,
			IO::MessageBoxButtons::Ok,
			IO::MessageBoxIcon::Error,
			Application::windowState.windowHandle
		);
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

		std::vector<Result> warnings;
		std::vector<Result> errors;

		std::for_each(std::execution::seq, filenames.begin(), filenames.end(),
			[this, &warnings, &errors, &m2](const auto& filename) {

				int id = nextPresetID++;
				NotesPreset preset(id, "");
				Result result = preset.read(filename);
				{
					std::lock_guard<std::mutex> lock{ m2 };

					if (result.getStatus() == ResultStatus::Success)
					{
						presets.emplace_back(std::move(preset));
					}
					else if (result.getStatus() == ResultStatus::Warning)
					{
						presets.emplace_back(std::move(preset));
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
			return Result(ResultStatus::Error, IO::formatString("Error: File '%s' not found", path));
		
		int id = nextPresetID++;
		NotesPreset preset = NotesPreset(id, "");
		Result result = preset.read(path);

		if (result.getStatus() == ResultStatus::Error)
			return result;
		
		fs::path dir{IO::File::getFilepath(path)};
		if (dir != presetsPath)
		{
			try
			{
				std::wstring wFilename{ IO::mbToWideStr(IO::File::getFilenameWithoutExtension(path)) };
				fs::copy_file(importPath, (presetsPath / wFilename).replace_extension(L".json"));
			}
			catch (const fs::filesystem_error& err)
			{
				return Result(ResultStatus::Error, err.what());
			}
		}
		
		presets.emplace_back(std::move(preset));
		return result;
	}

	void PresetManager::createPreset(const ScoreContext& context, const std::string &name, const std::string& desc)
	{
		if (context.selectedNotes.empty() || name.empty())
			return;

		NotesPreset preset(nextPresetID++, name);
		preset.description = desc;
		preset.data = jsonIO::noteSelectionToJson(context.score, context.selectedNotes, context.minTickFromSelection());

		try
		{
			if (createPresetFuture.valid())
				createPresetFuture.get();

			presets.push_back(preset);
			createPresetFuture = std::async(std::launch::async, &PresetManager::savePreset, this, preset);
		}
		catch (const std::exception& err)
		{
			showErrorMessage(IO::formatString("An error occurred while creating the preset: %s", err.what()));
		}
	}

	bool PresetManager::savePreset(NotesPreset preset)
	{
		if (fs::exists(presetsPath) || fs::create_directory(presetsPath))
		{
			// Extension is added after determining final file name
			preset.write(presetsPath / IO::mbToWideStr(fixFilename(preset.getName())), false);
			return true;
		}

		return false;
	}

	void PresetManager::removePreset(int index)
	{
		if (index < 0 || index >= presets.size())
			return;

		try
		{
			if (createPresetFuture.valid())
				createPresetFuture.get();

			if (deletePresetFuture.valid())
				deletePresetFuture.get();

			const auto& preset = presets[index];
			if (preset.getFilename().empty())
			{
				presets.erase(presets.begin() + index);
				return;
			}

			deletedPresetIndex = index;
			deletePresetFuture = std::async(std::launch::async, [this, preset]() -> bool
			{
				deletedPreset = std::move(preset);
				return fs::remove(presetsPath / IO::mbToWideStr(deletedPreset.getFilename()));
			});

			presets.erase(presets.begin() + index);
		}
		catch (const std::exception& err)
		{
			showErrorMessage(IO::formatString("An error occurred while removing the preset: %s", err.what()));
		}
	}

	bool PresetManager::undoDeletePreset()
	{
		if (fs::exists(presetsPath) || fs::create_directory(presetsPath))
		{
			try
			{
				deletedPreset.write(presetsPath / IO::mbToWideStr(deletedPreset.getFilename()), false);
				presets.insert(presets.begin() + deletedPresetIndex, std::move(deletedPreset));

				deletedPreset = {};
				return true;
			}
			catch (const std::exception& err)
			{
				showErrorMessage(IO::formatString("An error occurred while restoring the deleted preset: %s", err.what()));
			}
		}
		
		return false;
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

	void PresetManager::applyPreset(int index, ScoreContext& context)
	{
		if (index < 0 || index >= presets.size())
			return;

		applyPreset(presets.at(index), context);
	}

	void PresetManager::applyPreset(const NotesPreset& preset, ScoreContext& context)
	{
		const json& data = preset.data;
		if (jsonIO::arrayHasData(data, "notes") || jsonIO::arrayHasData(data, "holds"))
			context.doPasteData(data, false);
	}
}