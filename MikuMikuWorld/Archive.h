#pragma once
#include "zip.h"
#include "ArchiveError.h"
#include "ArchiveFile.h"
#include <string>

namespace IO
{
	enum class ArchiveOpenMode
	{
		None,
		Create = ZIP_CREATE,     // Create the a file if it doesn't exist
		Exclude = ZIP_EXCL,      // Don't open an existing file
		Strict = ZIP_CHECKCONS,  // Don't open if the file contains faulty data
		Truncate = ZIP_TRUNCATE, // Replace the current file
		Read = ZIP_RDONLY,       // Readonly
	};

	static ArchiveOpenMode operator|(const ArchiveOpenMode& op1, const ArchiveOpenMode& op2)
	{
		return static_cast<ArchiveOpenMode>(static_cast<int>(op1) | static_cast<int>(op2));
	}

	class Archive
	{
		using SourceStorage = std::vector<std::vector<uint8_t>>;
	public:
		bool isOpen() const;
		int64_t getNumOfEntries() const;
		const ArchiveError& getLastError() const;

		int64_t getEntryIndex(const std::string& entryName, bool caseSensitive = true, bool wildCard = false);
		const std::string& getEntryName(int64_t entryIndex);

		ArchiveFile openFile(const std::string& filename);
		ArchiveFile openFile(int64_t entryIndex);

		ArchiveFile createFile(const std::string& filename);
		void removeFile(const std::string& filename);
		
		void close();

		Archive(const std::string& archiveName, ArchiveOpenMode openmode = ArchiveOpenMode::None);
		Archive(const Archive&) = delete;
		~Archive();
	private:
		
		ArchiveError zipErr;
		zip_t* zipArchive;
		std::string archiveName;
		std::string entryName;
		std::shared_ptr<SourceStorage> storage;
	};

	std::string normalizeArchivePath(const std::string& filename);
	void normalizeArchivePath(std::string& filename);
}