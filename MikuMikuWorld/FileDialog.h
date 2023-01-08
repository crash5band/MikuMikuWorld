#pragma once
#include <string>
#include <nfd.h>

namespace MikuMikuWorld
{
	enum class FileType
	{
		ScoreFile,
		MMWSFile,
		SUSFile,
		AudioFile,
		ImageFile
	};

	class FileDialog
	{
	private:
		static const wchar_t* getDialogTitle(FileType type);
		static nfdfilteritem_t* getDialogFilters(FileType type);
		static int getFilterCount(FileType type);

	public:
		static bool openFile(std::string& name, FileType type);
		static bool saveFile(std::string& name, FileType type);
	};
}