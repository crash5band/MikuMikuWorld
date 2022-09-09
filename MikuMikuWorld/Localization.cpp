#include "Localization.h"
#include "StringOperations.h"
#include <filesystem>

namespace MikuMikuWorld
{
	std::unordered_map<std::string, Language> Localization::languages;
	Language Localization::currentLanguage;

	void Localization::readAll(const std::string& path)
	{
		Language en;
		en.read(path + "/en.csv");
		languages["en"] = en;

		Language jp;
		jp.read(path + "/jp.csv");
		languages["jp"] = jp;
	}

	void Localization::setLanguage(const std::string& code)
	{
		auto it = Localization::languages.find(code);
		if (it == Localization::languages.end())
			return;

		Localization::currentLanguage = it->second;
	}

	const std::string& getString(const std::string& key)
	{
		auto it = Localization::currentLanguage.strings.find(key);
		return it != Localization::currentLanguage.strings.end() ? it->second : "";
	}
}