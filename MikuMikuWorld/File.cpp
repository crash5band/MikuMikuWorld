#include "Platform.h"
#include "File.h"
#include "IO.h"
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>

namespace IO
{
	FileDialogFilter mmwsFilter{ "MikuMikuWorld Score", "*.mmws" };
	FileDialogFilter susFilter{ "Sliding Universal Score", "*.sus" };
	FileDialogFilter uscFilter{ "Sonolus Level", "*.usc" };
	FileDialogFilter imageFilter{ "Image Files", "*.jpg;*.jpeg;*.png" };
	FileDialogFilter audioFilter{ "Audio Files", "*.mp3;*.wav;*.flac;*.ogg" };
	FileDialogFilter allFilter{ "All Files", "*.*" };

	File::File(const std::string& filename, FileMode mode)
	{
		stream = std::make_unique<std::fstream>();
		open(filename, mode);
	}

	File::~File()
	{
		if (stream->is_open())
			stream->close();
	}

	std::ios::openmode File::getStreamMode(FileMode mode) const
	{
		switch (mode)
		{
		case FileMode::Read:
			return std::fstream::in;
		case FileMode::Write:
			return std::fstream::out;
		case FileMode::ReadBinary:
			return std::fstream::in | std::fstream::binary;
		case FileMode::WriteBinary:
			return std::fstream::out | std::fstream::binary;
		default:
			return std::ios::openmode(0);
		}
	}

	void File::open(const std::string& filename, FileMode mode)
	{
		openFilename = filename;
		stream->open(fs::path(filename), getStreamMode(mode));
	}

	void File::close()
	{
		openFilename.clear();
		stream->close();
	}

	void File::flush()
	{
		stream->flush();
	}

	std::vector<uint8_t> File::readAllBytes()
	{
		if (!stream->is_open())
			return {};

		stream->seekg(0, std::ios_base::end);
		size_t length = stream->tellg();
		stream->seekg(0, std::ios_base::beg);

		std::vector<uint8_t> bytes;
		bytes.resize(length);
		stream->read((char*)bytes.data(), length);

		return bytes;
	}

	std::string File::readLine()
	{
		if (!stream->is_open())
			return {};

		std::string line{};
		std::getline(*stream, line);
#ifndef MMW_WINDOWS
		if (const auto crPos = line.find('\r'); crPos != std::string::npos) {
			line = line.substr(0, crPos);
		}
#endif
		return line;
	}

	std::vector<std::string> File::readAllLines()
	{
		if (!stream->is_open())
			return {};

		std::vector<std::string> lines;
		while (!stream->eof())
			lines.push_back(readLine());

		return lines;
	}

	std::string File::readAllText()
	{
		if (!stream->is_open())
			return {};

		std::stringstream buffer;
		buffer << stream->rdbuf();
		return buffer.str();
	}

	bool File::isEndofFile()
	{
		return stream->is_open() ? stream->eof() : true;
	}

	void File::write(const std::string& str)
	{
		if (stream->is_open())
		{
			stream->write(str.c_str(), str.length());
		}
	}

	void File::writeLine(const std::string line)
	{
		write(line + "\n");
	}

	void File::writeAllLines(const std::vector<std::string>& lines)
	{
		if (stream->is_open())
		{
			std::stringstream ss{};
			for (const auto& line : lines)
				ss << line + '\n';

			std::string allLines{ ss.str() };
			stream->write(allLines.c_str(), allLines.size());
		}
	}

	void File::writeAllBytes(const std::vector<uint8_t>& bytes)
	{
		if (stream->is_open())
		{
			stream->write((char*)bytes.data(), bytes.size());
		}
	}

	std::string File::getFilename(const std::string& filename)
	{
		return fs::path(filename).filename().string();
	}

	std::string File::getFileExtension(const std::string& filename)
	{
		return fs::path(filename).extension().string();
	}

	std::string File::getFilenameWithoutExtension(const std::string& filename)
	{
		return fs::path(filename).stem().string();
	}

	std::string File::getFullFilenameWithoutExtension(const std::string& filename)
	{
		return fs::path(filename).replace_extension("").string();
	}

	std::string File::getFilepath(const std::string& filename)
	{
		return fs::path(filename).remove_filename().string();
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
		return fs::exists(path);
	}

	bool File::createDirectory(const std::string& path)
	{
		return fs::create_directory(path);
	}

	bool File::copyFile(const std::string& from, const std::string& to)
	{
		return fs::copy_file(from, to);
	}

	bool File::remove(const std::string& path)
	{
		return fs::remove(path);
	}

	fs::directory_iterator File::directoryIterator(const std::string& path)
	{
		return fs::directory_iterator{ path };
	}

	std::ifstream File::ifstream(const std::string& path)
	{
		return std::ifstream{ fs::path(path) };
	}

	std::ofstream File::ofstream(const std::string& path)
	{
		return std::ofstream{ fs::path(path) };
	}

	FileDialogResult FileDialog::showFileDialog(DialogType type, DialogSelectType selectType)
	{
		return Platform::OpenFileDialog(type, selectType, *this);
		// FIXME
		// return platform::showFileDialog(title, inputFilename, defaultExtension, filters, type, outputFilename);

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
