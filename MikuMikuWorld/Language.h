#pragma once
#include <string>
#include <unordered_map>

namespace MikuMikuWorld
{
	class Language
	{
	  private:
		std::string code;
		std::string displayName;
		std::unordered_map<std::string, std::string> strings;

	  public:
		Language(const char* code, std::string name, const std::string& filename);
		Language(const char* code, std::string name,
		         const std::unordered_map<std::string, std::string>& strings);

		void read(const std::string& filename);
		const char* getCode() const;
		const char* getDisplayName() const;
		const char* getString(const std::string& key) const;
	};
}