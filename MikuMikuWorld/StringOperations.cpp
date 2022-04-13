#include "StringOperations.h"
#include <Windows.h>

namespace MikuMikuWorld
{
	char* reverse(char* str)
	{
		char* end = str;
		char* start = str;

		if (!str || !*str) return str;
		while (*(end + 1)) end++;
		while (end > start)
		{
			int ch = *end;
			*end-- = *start;
			*start++ = ch;
		}
		return str;
	}

	char* tostringBaseN(char* buff, long long num, int base)
	{
		int sign = num < 0;
		char* savedbuff = buff;

		if (base < 2 || base >= sizeof(digits)) return NULL;
		if (buff)
		{
			do
			{
				*buff++ = digits[abs(num % base)];
				num /= base;
			} while (num);
			if (sign)
			{
				*buff++ = '-';
			}
			*buff = 0;
			reverse(savedbuff);
		}
		return savedbuff;
	}

	bool isComment(const std::string& line, const std::string& delim)
	{
		if (line.size() < 1)
			return true;

		return line.at(0) == '#' || line.at(0) == '\n';
	}

	bool startsWith(const std::string& line, const std::string& key)
	{
		return line.find_first_of(key) == 0;
	}

	bool endsWith(const std::string& line, const std::string& key)
	{
		return line.substr(line.size() - key.size()) == key;
	}

	bool isDigit(const std::string& str)
	{
		if (!str.size())
			return false;

		int index = 0;
		if (str.at(index) == '-')
			index = 1;

		for (; index < str.size(); ++index)
		{
			if (!isdigit(str[index]))
				return false;
		}

		return true;
	}

	std::string trim(std::string& line)
	{
		if (line.size() < 1)
			return line;

		size_t start = line.find_first_not_of(" ");
		size_t end = line.find_last_not_of(" ");

		return line.substr(start, end - start + 1);
	}

	std::vector<std::string> split(const std::string& line, const std::string& delim)
	{
		std::vector<std::string> values;
		size_t start = 0;
		size_t end = line.size() - 1;

		while (start < line.size() && end != std::string::npos)
		{
			end = line.find_first_of(delim, start);
			values.push_back(line.substr(start, end - start));

			start = end + 1;
		}

		return values;
	}

	std::string wideStringToMb(const std::wstring& str)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0, NULL, NULL);
		std::string result(size, 0);
		WideCharToMultiByte(CP_UTF8, 0, &str[0], (int)str.size(), &result[0], size, NULL, NULL);

		return result;
	}

	std::wstring mbToWideStr(const std::string& str)
	{
		int size = MultiByteToWideChar(CP_UTF8, 0, &str[0], str.size(), NULL, 0);
		std::wstring wResult(size, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], str.size(), &wResult[0], size);

		return wResult;
	}
}