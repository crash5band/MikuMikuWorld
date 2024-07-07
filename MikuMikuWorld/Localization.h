#pragma once
#include "Language.h"
#include <memory>

namespace MikuMikuWorld
{
	class Localization
	{
	  private:
	  public:
		static std::unordered_map<std::string, std::unique_ptr<Language>> languages;
		static Language* currentLanguage;

		static void load(const char* code, const std::string& filename);
		static bool setLanguage(const std::string& key);
		static void loadDefault();
		static void loadLanguages(const std::string& path);
	};

	const char* getString(const std::string& key);
}