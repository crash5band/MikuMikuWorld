#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

namespace IO
{
	enum class MessageBoxButtons : uint8_t
	{
		Ok,
		OkCancel,
		YesNo,
		YesNoCancel
	};

	enum class MessageBoxIcon : uint8_t
	{
		None,
		Information,
		Warning,
		Error,
		Question
	};

	enum class MessageBoxResult : uint8_t
	{
		None,
		Abort,
		Cancel,
		Ignore,
		No,
		Yes,
		Ok
	};

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

	std::string concat(const char* s1, const char* s2, const char* join = "");

	template<typename ... Args>
	std::string formatString(const char* format, Args ... args)
	{
		size_t length = std::snprintf(nullptr, 0, format, args ...) + 1;
		if (length <= 0)
			throw std::runtime_error("An error occured while attempting to format a string.");

		std::string buf(length, '0');
		std::snprintf(buf.data(), length, format, args ...);

		return buf;
	}

	MessageBoxResult messageBox(std::string title, std::string message, MessageBoxButtons buttons, MessageBoxIcon icon, void* parentWindow = NULL);
}