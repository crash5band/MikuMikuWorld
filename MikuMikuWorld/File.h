#pragma once
#include <string>
#include <vector>
#include <fstream>

namespace IO
{
	enum class FileMode : uint8_t
	{
		Read,
		Write,
		ReadBinary,
		WriteBinary
	};

	class File
	{
	public:
		static std::string getFilename(const std::string& filename);
		static std::string getFileExtension(const std::string& filename);
		static std::string getFilenameWithoutExtension(const std::string& filename);
		static std::string getFullFilenameWithoutExtension(const std::string& filename);
		static std::string getFilepath(const std::string& filename);
		static std::string fixPath(const std::string& path);
		static bool exists(const std::string& path);

		void open(const std::string& filename, FileMode mode);
		void close();
		void flush();

		std::vector<uint8_t> readAllBytes();
		std::string readLine();
		std::vector<std::string> readAllLines();
		std::string readAllText();
		void write(const std::string& str);
		void writeLine(const std::string line);
		void writeAllLines(const std::vector<std::string>& lines);
		void writeAllBytes(const std::vector<uint8_t>& bytes);
		bool isEndofFile();

		std::string_view getOpenFilename() const { return openFilename; }

		File(const std::string& filename, FileMode mode);
		~File();

	private:
		std::unique_ptr<std::fstream> stream{};
		std::string openFilename{};

		int getStreamMode(FileMode) const;
	};

	enum class FileDialogResult : uint8_t
	{
		Error,
		Cancel,
		OK
	};

	enum class DialogType : uint8_t
	{
		Open,
		Save
	};

	enum class DialogSelectType : uint8_t
	{
		File,
		Folder
	};

	struct FileDialogFilter
	{
		std::string filterName;
		std::string filterType;
	};

	class FileDialog
	{
	private:
		FileDialogResult showFileDialog(DialogType type, DialogSelectType selectType);

	public:
		std::string title;
		std::vector<FileDialogFilter> filters;
		std::string inputFilename;
		std::string outputFilename;
		std::string defaultExtension;
		uint32_t filterIndex = 0;
		void* parentWindowHandle = nullptr;

		FileDialogResult openFile();
		FileDialogResult saveFile();
	};

	extern FileDialogFilter mmwsFilter;
	extern FileDialogFilter susFilter;
	extern FileDialogFilter uscFilter;
	extern FileDialogFilter imageFilter;
	extern FileDialogFilter audioFilter;
	extern FileDialogFilter allFilter;
}
