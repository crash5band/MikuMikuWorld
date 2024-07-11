#include "File.h"
#include "IO.h"
#include <Windows.h>
#include <algorithm>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <chrono>

namespace IO
{
	File::File(const std::wstring& filename, const wchar_t* mode)
	{
		stream = NULL;
		open(filename, mode);
	}

	File::File(const std::string& filename, const char* mode)
	{
		stream = NULL;
		open(filename, mode);
	}

	File::File() { stream = NULL; }

	File::~File() { close(); }

	void File::open(const std::wstring& filename, const wchar_t* mode)
	{
		if (stream)
			close();

		stream = _wfopen(filename.c_str(), mode);
	}

	void File::open(const std::string& filename, const char* mode)
	{
		if (stream)
			close();

		stream = fopen(filename.c_str(), mode);
	}

	void File::close()
	{
		if (stream)
		{
			fclose(stream);
			stream = NULL;
		}
	}

	void File::flush()
	{
		if (stream)
			fflush(stream);
	}

	std::chrono::time_point<std::chrono::system_clock> File::getLastWriteTime() const
	{
		struct stat fileStat;
		fstat(_fileno(stream), &fileStat);
		auto time = fileStat.st_mtime;

		return std::chrono::system_clock::from_time_t(time);
	}

	std::vector<uint8_t> File::readAllBytes()
	{
		fseek(stream, 0, SEEK_END);
		size_t size = ftell(stream);
		fseek(stream, 0, SEEK_SET);

		std::vector<uint8_t> bytes;
		bytes.resize(size);
		fread(&bytes[0], sizeof(uint8_t), size, stream);

		return bytes;
	}

	std::string File::readLine() const
	{
		std::string line = "";
		while (!feof(stream))
		{
			char c;
			fread(&c, sizeof(uint8_t), 1, stream);
			if (feof(stream))
				break;

			if (c != '\n')
				line += c;
			else
				break;
		}

		return line;
	}

	std::vector<std::string> File::readAllLines() const
	{
		std::vector<std::string> lines;
		while (!isEndofFile())
			lines.push_back(readLine());

		return lines;
	}

	std::string File::readAllText() const
	{
		fseek(stream, 0, SEEK_END);
		int size = ftell(stream);
		fseek(stream, 0, SEEK_SET);

		std::string str(size, '0');
		fread(str.data(), sizeof(char), size, stream);

		return str;
	}

	bool File::isEndofFile() const
	{
		if (stream)
			return feof(stream);

		return true;
	}

	void File::write(const std::string& str) { fwrite(str.c_str(), str.size(), 1, stream); }

	void File::writeLine(const std::string line) { write(line + "\n"); }

	void File::writeAllLines(const std::vector<std::string>& lines)
	{
		for (const auto& line : lines)
			writeLine(line);
	}

	std::string File::getFilename(const std::string& filename)
	{
		size_t start = filename.find_last_of("\\/");
		return filename.substr(start + 1, filename.size() - (start + 1));
	}

	std::string File::getFileExtension(const std::string& filename)
	{
		size_t end = filename.find_last_of(".");
		if (end == std::string::npos)
			return "";

		return filename.substr(end);
	}

	std::string File::getFilenameWithoutExtension(const std::string& filename)
	{
		std::string str = getFilename(filename);
		size_t end = str.find_last_of(".");

		return str.substr(0, end);
	}

	std::string File::getFilepath(const std::string& filename)
	{
		size_t start = 0;
		size_t end = filename.find_last_of("\\/");

		return filename.substr(start, end - start + 1);
	}

	std::string File::fixPath(const std::string& path)
	{
		std::string result = path;
		int index = 0;
		while (true)
		{
			index = result.find("\\", index);
			if (index == result.npos)
				break;

			result.replace(index, 1, "/");
			index += 1;
		}

		return result;
	}

	bool File::exists(const std::string& path)
	{
		std::wstring wPath = mbToWideStr(path);
		return std::filesystem::exists(wPath);
	}

	bool File::exists(const std::wstring& path)
	{
		return std::filesystem::exists(path);
	}

	FileDialogResult FileDialog::showFileDialog(DialogType type, DialogSelectType selectType)
	{
		std::wstring wTitle = mbToWideStr(title);

		OPENFILENAMEW ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = reinterpret_cast<HWND>(parentWindowHandle);
		ofn.lpstrTitle = wTitle.c_str();
		ofn.nFilterIndex = filterIndex + 1;
		ofn.nFileOffset = 0;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT |
		            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;

		std::wstring wDefaultExtension = mbToWideStr(defaultExtension);
		ofn.lpstrDefExt = wDefaultExtension.c_str();

		std::vector<std::wstring> ofnFilters;
		ofnFilters.reserve(filters.size());

		/*
		    since '\0' terminates the string,
		    we'll do a C# by using ' | ' then replacing it with '\0' when constructing the final
		   wide string
		*/
		std::string filtersCombined;
		for (const auto& filter : filters)
		{
			filtersCombined.append(filter.filterName)
			    .append(" (")
			    .append(filter.filterType)
			    .append(")|")
			    .append(filter.filterType)
			    .append("|");
		}

		std::wstring wFiltersCombined = mbToWideStr(filtersCombined);
		std::replace(wFiltersCombined.begin(), wFiltersCombined.end(), '|', '\0');
		ofn.lpstrFilter = wFiltersCombined.c_str();

		std::wstring wInputFilename = mbToWideStr(inputFilename);
		wchar_t ofnFilename[1024]{ 0 };

		// suppress return value not used warning
#pragma warning(suppress : 6031)
		lstrcpynW(ofnFilename, wInputFilename.c_str(), 1024);
		ofn.lpstrFile = ofnFilename;

		if (type == DialogType::Save)
		{
			if (GetSaveFileNameW(&ofn))
			{
				outputFilename = wideStringToMb(ofn.lpstrFile);
			}
			else
			{
				// user canceled
				return FileDialogResult::Cancel;
			}
		}
		else if (GetOpenFileNameW(&ofn))
		{
			outputFilename = wideStringToMb(ofn.lpstrFile);
		}
		else
		{
			return FileDialogResult::Cancel;
		}

		return outputFilename.empty() ? FileDialogResult::Cancel : FileDialogResult::OK;
	}

	FileDialogResult FileDialog::openFile()
	{
		return showFileDialog(DialogType::Open, DialogSelectType::File);
	}

	FileDialogResult FileDialog::saveFile()
	{
		return showFileDialog(DialogType::Save, DialogSelectType::File);
	}
}
