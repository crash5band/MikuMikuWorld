#pragma once
#include "zip.h"
#include "ArchiveError.h"
#include <memory>
#include <vector>

namespace IO
{
    class ArchiveFile
    {
        using SourceStorage = std::vector<std::vector<uint8_t>>;
    public:
        bool isOpen() const;
        const ArchiveError& getLastError() const;

        void close();

		std::vector<uint8_t> readAllBytes();
		std::vector<std::string> readAllLines();
		std::string readAllText();
        void writeAllText(const std::string& text);
		void writeAllLines(const std::vector<std::string>& lines);
        // copy, specify if the buffer needed to be copy into a special storage
        // if copy is false, you must not free the buffer BEFORE freeing the archive
		void writeAllBytes(const std::vector<uint8_t>& bytes, bool copy = true); 

        ArchiveFile();
        ArchiveFile(const ArchiveFile&) = delete;
        ~ArchiveFile();
    private:
        ArchiveFile(zip_t* archive, zip_file_t* file, zip_int64_t index, const std::shared_ptr<SourceStorage>& storage);
        ArchiveError zipErr;
        zip_t* zipArchive;
        zip_file_t* zipFile;
        zip_int64_t fileIndex;
        std::shared_ptr<SourceStorage> storage;

        friend class Archive;
    };
}