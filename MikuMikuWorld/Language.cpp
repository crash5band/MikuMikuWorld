#include "Language.h"
#include "IO.h"
#include "File.h"

using namespace IO;

namespace MikuMikuWorld
{
	static std::string empty;

	Language::Language(const char* code, std::string name, const std::string& filename)
	{
		this->code = code;
		displayName = name;
		read(filename);
	}

	Language::Language(const char* code, std::string name,
	                   const std::unordered_map<std::string, std::string>& strings)
	{
		this->code = code;
		displayName = name;
		this->strings = strings;
	}

	void Language::read(const std::string& filename)
	{
		if (!File::exists(filename))
			return;

		File f(mbToWideStr(filename), L"r");
		std::vector<std::string> lines = f.readAllLines();

		f.close();
		strings.reserve(lines.size());

		for (auto& line : lines)
		{
			line = trim(line);
			if (!line.size() || startsWith(line, "#"))
				continue;

			std::vector<std::string> values = split(line, ",");
			strings[trim(values[0])] = trim(values[1]);
		}
	}

	const char* Language::getCode() const { return code.c_str(); }

	const char* Language::getDisplayName() const { return displayName.c_str(); }

	const bool Language::containsString(const std::string& key)
	{
		return strings.find(key) != strings.end();
	}

	const char* Language::getString(const std::string& key) const
	{
		const auto& it = strings.find(key);

		// imgui dies if the window/header title is empty
		return it != strings.end() ? it->second.c_str() : key.c_str();
	}
}