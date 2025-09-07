#include "Archive.h"
#include <filesystem>

namespace IO
{
	Archive::Archive() : zipArchive(NULL), zipErr(), storage(std::make_shared<Archive::SourceStorage>())
	{

	}

	Archive::Archive(const std::string& archiveName, ArchiveOpenMode openmode) : zipArchive(NULL), zipErr(), storage(std::make_shared<Archive::SourceStorage>())
	{
		int err;
		zipArchive = zip_open(archiveName.c_str(), static_cast<int>(openmode), &err);
		if (zipArchive == NULL) zipErr.set(err);
	}

	Archive::~Archive()
	{
		this->close();
	}

	bool Archive::isOpen() const
	{
		return zipArchive != NULL;
	}

	int64_t Archive::getNumOfEntries() const
	{
		return zip_get_num_entries(zipArchive, 0);
	}

	const ArchiveError& IO::Archive::getLastError() const
	{
		return zipErr;
	}

	int64_t Archive::getEntryIndex(const std::string& entryName, bool caseSensitive, bool wildCard)
	{
		if (!isOpen())
		{
			zipErr.set(ZIP_ER_ZIPCLOSED);
			return -1;
		}
		else
			zipErr.set(ZIP_ER_OK);
		zip_flags_t flag = (!caseSensitive ? ZIP_FL_NOCASE : 0) | (wildCard ? ZIP_FL_NODIR : 0);
		return zip_name_locate(zipArchive, entryName.c_str(), flag);
	}

	std::string Archive::getEntryName(int64_t entryIndex)
	{
		std::string entryName;
		if (!isOpen())
		{
			zipErr.set(ZIP_ER_ZIPCLOSED);
			return entryName;
		}
		const char* name = zip_get_name(zipArchive, entryIndex, 0);
		if (name == NULL)
		{
			zipErr.set(zip_get_error(zipArchive));
			return entryName;
		}
		entryName = name;
		zipErr.set(ZIP_ER_OK);
		return entryName;
	}

	ArchiveFile Archive::openFile(const std::string& filename)
	{
		if (!isOpen())
		{
			zipErr.set(ZIP_ER_ZIPCLOSED);
			return {};
		}
		zip_int64_t idx = zip_name_locate(zipArchive, filename.c_str(), 0);
		if (idx >= 0)
			return openFile(idx);
		else
			return createFile(filename);
	}

	ArchiveFile Archive::openFile(int64_t entryIndex)
	{
		if (!isOpen())
		{
			zipErr.set(ZIP_ER_ZIPCLOSED);
			return {};
		}
		zip_file_t* zipFile = zip_fopen_index(zipArchive, entryIndex, 0);
		if (zipFile == NULL)
		{
			zipErr.set(zip_get_error(zipArchive));
			return {};
		}
		zipErr.set(ZIP_ER_OK);
		return { zipArchive, zipFile, entryIndex, storage };
	}

	ArchiveFile Archive::createFile(const std::string& filename)
	{
		static const char EMPTY_BUFF[1] = "";
		if (!isOpen())
		{
			zipErr.set(ZIP_ER_ZIPCLOSED);
			return {};
		}
		zip_source_t* source = zip_source_buffer(zipArchive, EMPTY_BUFF, 0, 0);
		if (source == NULL)
		{
			zipErr.set(zip_get_error(zipArchive));
			return {};
		}
		zip_int64_t fileIdx = zip_file_add(zipArchive, filename.c_str(), source, 0);
		if (fileIdx < 0)
		{
			zip_source_free(source);
			zipErr.set(zip_get_error(zipArchive));
			return {};
		}
		return openFile(fileIdx);
	}

	void Archive::removeFile(const std::string& filename)
	{
		zip_int64_t idx = zip_name_locate(zipArchive, filename.c_str(), 0);
		if (idx < 0)
			return;
		if (zip_delete(zipArchive, idx) < 0)
		{
			zipErr.set(zip_get_error(zipArchive));
			return;
		}
		zipErr.set(ZIP_ER_OK);
	}

	void Archive::open(const std::string& archiveName, ArchiveOpenMode openmode)
	{
		int err;
		zipArchive = zip_open(archiveName.c_str(), static_cast<int>(openmode), &err);
		if (zipArchive == NULL) zipErr.set(err);
	}

	void Archive::close()
	{
		if (isOpen())
		{
			zip_close(zipArchive);
			zipArchive = NULL;
		}
	}
}