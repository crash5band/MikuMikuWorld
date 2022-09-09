#include "Localization.h"
#include "StringOperations.h"
#include <filesystem>

namespace MikuMikuWorld
{
	std::unordered_map<std::string, std::unique_ptr<Language>> Localization::languages;
	Language* Localization::currentLanguage = nullptr;

	void Localization::readAll(const std::string& path)
	{
        languages.reserve(2);
        languages["en"] = std::make_unique<Language>("en", path + "/en.csv");
        languages["jp"] = std::make_unique<Language>("jp", path + "/jp.csv");
	}

	void Localization::setLanguage(const std::string& code)
	{
		auto it = Localization::languages.find(code);
		if (it == Localization::languages.end())
			return;

		Localization::currentLanguage = it->second.get();
	}

	const std::string& getString(const std::string& key)
	{
        if (!Localization::currentLanguage)
            return "";

        return Localization::currentLanguage->getString(key);
	}
}