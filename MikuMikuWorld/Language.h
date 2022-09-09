#pragma once
#include <unordered_map>

namespace MikuMikuWorld
{
	class Language
	{
	public:
		std::string code;
		std::unordered_map<std::string, std::string> strings;

		void read(const std::string& filename);
	};
}