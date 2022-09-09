#pragma once
#include "Language.h"

namespace MikuMikuWorld
{
	class Localization
	{
	public:
		static std::unordered_map<std::string, Language> languages;
		static Language currentLanguage;

		static void readAll(const std::string& path);
		static void readOne(const std::string& filename);
		static void setLanguage(const std::string& key);
	};

	const std::string& getString(const std::string& key);
}