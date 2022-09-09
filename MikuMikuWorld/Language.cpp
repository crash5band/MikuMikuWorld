#include "Language.h"
#include "StringOperations.h"
#include "File.h"

namespace MikuMikuWorld
{
	void Language::read(const std::string& filename)
	{
		if (!File::exists(filename))
			return;

		std::wstring wFilename = mbToWideStr(filename);
		
		File f(wFilename, L"r");
		std::vector<std::string> lines = f.readAllLines();

		f.close();

		for (auto& line : lines)
		{
			line = trim(line);
			if (!line.size() || startsWith(line, "#"))
				continue;

			std::vector<std::string> values = split(line, ",");
			strings[trim(values[0])] = trim(values[1]);
		}
	}
}