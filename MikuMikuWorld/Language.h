#pragma once
#include <unordered_map>

namespace MikuMikuWorld
{
	class Language
	{
    private:
        std::string code;
        std::unordered_map<std::string, std::string> strings;

	public:
        Language(const std::string& code, const std::string& filename);

		void read(const std::string& filename);
        const std::string& getCode() const;
        const std::string& getString(const std::string& key) const;
	};
}