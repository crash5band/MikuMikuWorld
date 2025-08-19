#include "ArchiveFile.h"
#include <cassert>
#include <numeric>
#include <iterator>

namespace IO
{
	ArchiveFile::ArchiveFile() : zipErr(), zipArchive(NULL), zipFile(NULL), fileIndex(-1) { }

	ArchiveFile::ArchiveFile(zip_t* archive, zip_file_t* file, zip_int64_t index, const std::shared_ptr<SourceStorage>& storage) : zipErr(), zipArchive(archive), zipFile(file), fileIndex(index), storage(storage) { }

	ArchiveFile::~ArchiveFile()
	{
		this->close();
	}
	
	bool ArchiveFile::isOpen() const
	{
		return zipFile != NULL;
	}

	const ArchiveError& ArchiveFile::getLastError() const
	{
		return zipErr;
	}

    void ArchiveFile::close()
	{
		if (zipFile != NULL)
		{
			zip_fclose(zipFile);
			zipFile = NULL;
			zipFile = NULL;
			fileIndex = -1;
		}
	}

	std::vector<uint8_t> ArchiveFile::readAllBytes()
	{
		assert(isOpen());
		zip_stat_t fileStat;
		zip_stat_init(&fileStat);
		zip_stat_index(zipArchive, fileIndex, 0, &fileStat);
		std::vector<uint8_t> buffer(fileStat.size);
		for (zip_int64_t totalRead = 0, read = 0; totalRead < fileStat.size; totalRead += read)
		{
			read = zip_fread(zipFile, buffer.data() + totalRead, fileStat.size - totalRead);
			if (read < 0)
			{
				zipErr.set(zip_file_get_error(zipFile));
				return {};
			}
			if (read == 0)
			{
				buffer.resize(totalRead);
				break;
			}
		}
		zipErr.set(ZIP_ER_OK);
		return buffer;
	}
	
	std::vector<std::string> ArchiveFile::readAllLines()
	{
		assert(isOpen());
		std::string text = readAllText();
		if (zipErr.get() != ZIP_ER_OK)
			return {};
		std::vector<std::string> lines;
		for (size_t idx = 0; idx < text.size(); )
		{
			auto lineStart = std::next(text.begin(), idx);
			auto lineEnd = std::find(lineStart, text.end(), '\n');
			idx += std::distance(lineStart, lineEnd) + 1;
			if (lineEnd != text.begin() && *std::prev(lineEnd) == '\r')
				lineEnd = std::prev(lineEnd);
			lines.emplace_back(lineStart, lineEnd);
		}
		return lines;
	}
	
	std::string ArchiveFile::readAllText()
	{
		assert(isOpen());
		zip_stat_t fileStat;
		zip_stat_init(&fileStat);
		zip_stat_index(zipArchive, fileIndex, 0, &fileStat);
		std::string text(fileStat.size, '\0');
		for (zip_int64_t totalRead = 0, read = 0; totalRead < fileStat.size; totalRead += read)
		{
			read = zip_fread(zipFile, text.data() + totalRead, fileStat.size - totalRead);
			if (read < 0)
			{
				zipErr.set(zip_file_get_error(zipFile));
				return {};
			}
			if (read == 0)
			{
				text.resize(totalRead);
				break;
			}
		}
		zipErr.set(ZIP_ER_OK);
		return text;
	}

	void ArchiveFile::writeAllText(const std::string& text)
	{
		assert(isOpen());
		writeAllBytes(std::vector<uint8_t>{ text.begin(), text.end() });
	}

	void ArchiveFile::writeAllLines(const std::vector<std::string>& lines)
	{
		assert(isOpen());
		std::vector<uint8_t> data;
		if (lines.size())
		{
			data.reserve(std::accumulate(lines.begin(), lines.end(), size_t(0), [](size_t i, const std::string& s) { return i + s.size(); }) + lines.size() - 1);
			for (auto& line : lines)
			{
				std::copy(line.begin(), line.end(), std::back_inserter(data));
				data.push_back('\n');
			}
		}
		writeAllBytes(data);
	}

	void ArchiveFile::writeAllBytes(const std::vector<uint8_t>& bytes, bool copy)
	{
		assert(isOpen());
		const void* buffData;
		if (copy)
		{
			storage->push_back(bytes);
			buffData = storage->back().data();
		}
		else
			buffData = bytes.data();
		zip_source* source = zip_source_buffer(zipArchive, buffData, bytes.size(), 0);
		if (source == NULL)
		{
			zipErr.set(zip_get_error(zipArchive));
			return;
		}
		if (zip_file_replace(zipArchive, fileIndex, source, 0) < 0)
		{
			zip_source_free(source);
			zipErr.set(zip_get_error(zipArchive));
			return;
		}
		zipErr.set(ZIP_ER_OK);
		zip_file_set_mtime(zipArchive, fileIndex, time(nullptr), 0);
	}
}