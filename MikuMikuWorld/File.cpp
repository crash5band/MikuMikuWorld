#include "File.h"
#include "StringOperations.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>

namespace MikuMikuWorld
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

	File::File()
	{
		stream = NULL;
	}

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
		fclose(stream);
		stream = NULL;
	}

	void File::flush()
	{
		if (stream)
			fflush(stream);
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

		char* buf = new char[size];
		fread(buf, sizeof(char), size, stream);

		std::string result(buf);

		delete[] buf;

		return result;
	}

	bool File::isEndofFile() const
	{
		if (stream)
			return feof(stream);

		return true;
	}

	void File::write(const std::string str)
	{
		fwrite(str.c_str(), str.size(), 1, stream);
	}

	void File::writeLine(const std::string line)
	{
		write(line);
		fwrite("\n", 1, 1, stream);
	}

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
			throw std::runtime_error("Cannot get the extension of a file without extension.");

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
}