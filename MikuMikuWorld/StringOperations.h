#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

namespace MikuMikuWorld
{
    constexpr const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    char* reverse(char* str);
	char* tostringBaseN(char* buff, long long num, int base);
	bool isComment(const std::string& line, const std::string& delim);
	bool startsWith(const std::string& line, const std::string& key);
	bool endsWith(const std::string& line, const std::string& key);
	bool isDigit(const std::string& str);
	std::string trim(std::string& line);
	std::vector<std::string> split(const std::string& line, const std::string& delim);

	std::string wideStringToMb(const std::wstring& str);
	std::wstring mbToWideStr(const std::string& str);

	template<typename ... Args>
	std::string formatString(const char* format, Args ... args)
	{
		size_t length = std::snprintf(nullptr, 0, format) + 1;
		if (length <= 0)
			throw std::runtime_error("An error occured while attempting to format a string.");

		std::unique_ptr<char[]> buf(new char[length]);
		std::snprintf(buf.get(), length, format, args ...);

		return std::string(buf.get());
	}
}