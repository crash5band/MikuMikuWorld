#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include "Platform.h"

#define Z_CHUNK_SIZE 32768ULL

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
	bool isComment(const std::string_view& line, const std::string_view& delim);
	bool startsWith(const std::string_view& line, const std::string_view& key);
	bool endsWith(const std::string_view& line, const std::string_view& key);
	bool isDigit(const std::string_view& str);
	std::string trim(const std::string& line);
	std::vector<std::string> split(const std::string& line, const std::string& delim);

#ifdef MMW_WINDOWS
	std::string wideStringToMb(const std::wstring& str);
	std::wstring mbToWideStr(const std::string& str);
#endif

	std::string concat(const char* s1, const char* s2, const char* join = "");

	std::vector<uint8_t> inflateGzip(const std::vector<uint8_t>& data);
	std::vector<uint8_t> deflateGzip(const std::vector<uint8_t>& data);
	bool isGzipCompressed(const std::vector<uint8_t>& data);

	template<typename ... Args>
	std::string formatString(const char* format, Args ... args)
	{
		size_t length = std::snprintf(nullptr, 0, format, args ...) + 1;
		if (length <= 0)
			throw std::runtime_error("An error occured while attempting to format a string.");

		std::unique_ptr<char[]> buf(new char[length]);
		std::snprintf(buf.get(), length, format, args ...);

		return std::string(buf.get());
	}

	MessageBoxResult messageBox(std::string title, std::string message, MessageBoxButtons buttons, MessageBoxIcon icon, void* parentWindow = NULL);
}