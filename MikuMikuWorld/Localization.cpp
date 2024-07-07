#include "Localization.h"
#include "DefaultLanguage.h"
#include "IO.h"
#include "File.h"
#include <filesystem>

namespace MikuMikuWorld
{
	static std::string empty;

	std::unordered_map<std::string, std::unique_ptr<Language>> Localization::languages;
	Language* Localization::currentLanguage = nullptr;

	void Localization::load(const char* code, const std::string& filename)
	{
		if (!IO::File::exists(filename))
			return;

		languages[code] = std::make_unique<Language>(code, filename);
	}

	bool Localization::setLanguage(const std::string& code)
	{
		auto it = Localization::languages.find(code);
		if (it == Localization::languages.end())
			return false;

		Localization::currentLanguage = it->second.get();
		return true;
	}

	void Localization::loadDefault() { languages["en"] = std::make_unique<Language>("en", en); }

	const char* getString(const std::string& key)
	{
		if (!Localization::currentLanguage)
			return key.c_str();

		if (!Localization::currentLanguage->containsString(key))
			return Localization::languages["en"]->getString(key);

		return Localization::currentLanguage->getString(key);
	}

	void Localization::loadLanguages(const std::string& path)
	{
		std::wstring wPath = IO::mbToWideStr(path);
		if (!std::filesystem::exists(wPath))
			return;

		std::vector<std::filesystem::path> filePaths;
		for (const auto& file : std::filesystem::directory_iterator(wPath))
		{
			// look only for csv files and ignore any dot files present
			std::wstring wFilename = file.path().filename().wstring();
			if (file.path().extension().wstring() == L".csv" && wFilename[0] != L'.')
				filePaths.push_back(file.path());
		}

		for (const auto& filePath : filePaths)
		{
			auto countryCode = IO::wideStringToMb(filePath.stem().wstring());
			auto path = IO::wideStringToMb(filePath.wstring());
			Localization::load(countryCode.c_str(), path.c_str());
		}
	}
}