#include "Language.h"
#include "StringOperations.h"
#include "File.h"

namespace MikuMikuWorld
{
    Language::Language(const std::string& code, const std::string& filename)
    {
        this->code = code;
        read(filename);
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

    const std::string& Language::getCode() const
    {
        return code;
    }

    const std::string Language::getString(const std::string& key) const
    {
        const auto& it = strings.find(key);
		return it != strings.end() ? it->second : "";
    }
}