#include "Localization.h"
#include "DefaultLanguage.h"
#include "StringOperations.h"
#include "File.h"
#include <filesystem>

namespace MikuMikuWorld
{
	static std::string empty;

	std::unordered_map<std::string, std::unique_ptr<Language>> Localization::languages;
	Language* Localization::currentLanguage = nullptr;

	void Localization::load(const char* code, const std::string& filename)
	{
		if (!File::exists(filename))
			return;

        languages[code] = std::make_unique<Language>(code, filename);
	}

	void Localization::setLanguage(const std::string& code)
	{
		auto it = Localization::languages.find(code);
		if (it == Localization::languages.end())
			return;

		Localization::currentLanguage = it->second.get();
	}

	void Localization::loadDefault()
	{
		languages["en"] = std::make_unique<Language>("en", en);
	}

	const char* getString(const std::string& key)
	{
        if (!Localization::currentLanguage)
            return empty.c_str();

        return Localization::currentLanguage->getString(key);
	}
}