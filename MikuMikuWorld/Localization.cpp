#include "Localization.h"
#include "DefaultLanguage.h"
#include "IO.h"
#include "File.h"
#include <filesystem>

namespace MikuMikuWorld
{
	static std::string empty;

	std::map<std::string, std::unique_ptr<Language>> Localization::languages;
	Language* Localization::currentLanguage = nullptr;

	void Localization::load(const char* code, std::string name, const std::string& filename)
	{
		if (!IO::File::exists(filename))
			return;

        languages[code] = std::make_unique<Language>(code, name, filename);
	}

	bool Localization::setLanguage(const std::string& code)
	{
		size_t pos = 0;
		while (true) {
			size_t end_pos = code.find(':', pos); // support preference code like fr:ja:en
			std::string language = end_pos == std::string::npos ? code.substr(pos) : code.substr(pos, end_pos - pos);
			auto it = Localization::languages.find(language);
			if (it != Localization::languages.end()) {
				Localization::currentLanguage = it->second.get();
				return true;
			}
			if (end_pos == std::string::npos)
				break;
			pos = end_pos + 1;
		}
		return false;
	}

	void Localization::loadDefault()
	{
		languages["en"] = std::make_unique<Language>("en", "English", en);
	}

	const char* getString(const std::string& key)
	{
		if (!Localization::currentLanguage)
			return key.c_str();

        return Localization::currentLanguage->getString(key);
	}
}