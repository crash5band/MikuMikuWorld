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

	Language::Language(const char* code, std::string name, const std::map<std::string, std::string>& strings)
	{
		this->code = code;
		displayName = name;
		this->strings = strings;
	}

	void Language::read(const std::string& filename)
	{
		if (!File::exists(filename))
			return;

		File f(filename, FileMode::Read);
		std::vector<std::string> lines = f.readAllLines();

		f.close();        

		for (auto& line : lines)
		{
			line = trim(line);
			if (!line.size() || startsWith(line, "#"))
				continue;

			size_t start = 0, end = std::min(line.length() - 1, line.find_first_of(","));
			
			std::string key = line.substr(start, end - start);
			std::string value = line.substr(end + 1);

			strings[trim(key)] = trim(value);
		}
	}

    const char* Language::getCode() const
    {
        return code.c_str();
    }

	const char* Language::getDisplayName() const
	{
		return displayName.c_str();
	}

    const char* Language::getString(const std::string& key) const
    {
        const auto& it = strings.find(key);

		// imgui dies if the window/header title is empty
		return it != strings.end() ? it->second.c_str() : key.c_str();
    }
}