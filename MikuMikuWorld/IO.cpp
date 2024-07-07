#include "IO.h"
#include <Windows.h>
#include <algorithm>

namespace IO
{
	MessageBoxResult messageBox(std::string title, std::string message, MessageBoxButtons buttons,
	                            MessageBoxIcon icon, void* parentWindow)
	{
		UINT flags = 0;
		switch (icon)
		{
		case MessageBoxIcon::Information:
			flags |= MB_ICONINFORMATION;
			break;
		case MessageBoxIcon::Warning:
			flags |= MB_ICONWARNING;
			break;
		case MessageBoxIcon::Error:
			flags |= MB_ICONERROR;
			break;
		case MessageBoxIcon::Question:
			flags |= MB_ICONQUESTION;
			break;
		default:
			break;
		}

		switch (buttons)
		{
		case MessageBoxButtons::Ok:
			flags |= MB_OK;
			break;
		case MessageBoxButtons::OkCancel:
			flags |= MB_OKCANCEL;
			break;
		case MessageBoxButtons::YesNo:
			flags |= MB_YESNO;
			break;
		case MessageBoxButtons::YesNoCancel:
			flags |= MB_YESNOCANCEL;
			break;
		default:
			break;
		}

		const int result =
		    MessageBoxExW(reinterpret_cast<HWND>(parentWindow), mbToWideStr(message).c_str(),
		                  mbToWideStr(title).c_str(), flags, 0);
		switch (result)
		{
		case IDABORT:
			return MessageBoxResult::Abort;
		case IDCANCEL:
			return MessageBoxResult::Cancel;
		case IDIGNORE:
			return MessageBoxResult::Ignore;
		case IDNO:
			return MessageBoxResult::No;
		case IDYES:
			return MessageBoxResult::Yes;
		case IDOK:
			return MessageBoxResult::Ok;
		default:
			return MessageBoxResult::None;
		}
	}

	char* reverse(char* str)
	{
		char* end = str;
		char* start = str;

		if (!str || !*str)
			return str;
		while (*(end + 1))
			end++;
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

		if (base < 2 || base >= sizeof(digits))
			return NULL;
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

	bool isComment(const std::string_view& line, const std::string_view& delim)
	{
		if (line.empty())
			return true;

		return line.find_first_of(delim) == 0 || line.at(0) == '\n';
	}

	bool startsWith(const std::string_view& line, const std::string_view& key)
	{
		return std::equal(key.begin(), key.end(), line.begin());
	}

	bool endsWith(const std::string_view& line, const std::string_view& key)
	{
		return std::equal(key.rbegin(), key.rend(), line.rbegin());
	}

	bool isDigit(const std::string_view& str)
	{
		if (str.empty())
			return false;

		return std::all_of(str.begin() + (str.at(0) == '-' ? 1 : 0), str.end(), std::isdigit);
	}

	std::string trim(const std::string& line)
	{
		if (line.empty())
			return line;

		size_t start = line.find_first_not_of(" ");
		size_t end = line.find_last_not_of(" ");

		return line.substr(start, end - start + 1);
	}

	std::vector<std::string> split(const std::string& line, const std::string& delim)
	{
		std::vector<std::string> values;
		size_t start = 0;
		size_t end = line.length() - 1;

		while (start < line.length() && end != std::string::npos)
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

	std::string concat(const char* s1, const char* s2, const char* join)
	{
		return std::string(s1).append(join).append(s2);
	}
}