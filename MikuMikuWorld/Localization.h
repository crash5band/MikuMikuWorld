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

		static void readAll(const std::string& path);
		static void setLanguage(const std::string& key);
	};

	const std::string getString(const std::string& key);
}