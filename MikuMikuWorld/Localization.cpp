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

	void Localization::load(const char* code, std::string name, const std::string& filename)
	{
		if (!IO::File::exists(filename))
			return;

        languages[code] = std::make_unique<Language>(code, name, filename);
	}

	bool Localization::setLanguage(const std::string& code)
	{
		auto it = Localization::languages.find(code);
		if (it == Localization::languages.end())
			return false;

		Localization::currentLanguage = it->second.get();
		return true;
	}

	void Localization::loadDefault()
	{
		languages["en"] = std::make_unique<Language>("en", "English", en);
	}

	const char* getString(const std::string& key)
	{
        if (!Localization::currentLanguage)
            return empty.c_str();

        return Localization::currentLanguage->getString(key);
	}
}