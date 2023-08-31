#include "Preset.h"
#include "File.h"
#include "IO.h"
#include <fstream>
#include <filesystem>

using nlohmann::json;

namespace MikuMikuWorld
{
	NotesPreset::NotesPreset(int _id, std::string _name) :
		ID{ _id }, name{ _name }
	{

	}

	NotesPreset::NotesPreset() : ID{ -1 }, name{ "" }, description{ "" }
	{

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
		if (data.find("name") != data.end())
			name = data["name"];

		if (data.find("description") != data.end())
			description = data["description"];

		if (data.find("notes") == data.end() && data.find("holds") == data.end())
			return Result(ResultStatus::Warning, "The preset " + name + " does not contain any notes data. Skipping...");

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
		file.close();
	}
}
