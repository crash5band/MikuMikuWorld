#include "FileDialog.h"
#include "StringOperations.h"
#include <nfd.h>
#include <Windows.h>

namespace MikuMikuWorld
{
	const wchar_t* FileDialog::getDialogTitle(FileType type)
	{
		switch (type)
		{
		case FileType::ScoreFile:
			return L"Score File";
		case FileType::MMWSFile:
			return L"Miku Miku World Score";
		case FileType::SUSFile:
			return L"Sliding Universal Score";
		case FileType::AudioFile:
			return L"Audio File";
		case FileType::ImageFile:
			return L"Image File";
		default:
			return L"File";
		}
	}

	int FileDialog::getFilterCount(FileType type)
	{
		switch (type)
		{
		case MikuMikuWorld::FileType::ScoreFile:
			return 3;
		case MikuMikuWorld::FileType::MMWSFile:
			return 1;
		case MikuMikuWorld::FileType::SUSFile:
			return 1;
		case MikuMikuWorld::FileType::AudioFile:
			return 5;
		case MikuMikuWorld::FileType::ImageFile:
			return 3;
		default:
			return 0;
		}
	}

	nfdfilteritem_t* FileDialog::getDialogFilters(FileType type)
	{
		switch (type)
		{
		case FileType::ScoreFile:
			return new nfdfilteritem_t[3]{ 
				{"Score Files", "mmws,sus"},
				{"MikuMikuWorld Score", "mmws"},
				{"Sliding Universal Score", "sus"}
			};
		case FileType::MMWSFile:
			return new nfdfilteritem_t[1]{ {"MikuMikuWorld Score", "mmws"} };
		case FileType::SUSFile:
			return new nfdfilteritem_t[1]{ {"Sliding Universal Score", "sus"} };
		case FileType::AudioFile:
			return new nfdfilteritem_t[5]{
				{"Audio Files", "mp3,wav,flac,ogg"},
				{"MP3 Files", "mp3"},
				{"WAV Files", "wav"},
				{"FLAC Files", "flac"},
				{"OGG Vorbis Files", "ogg"}
			};
		case FileType::ImageFile:
			return new nfdfilteritem_t[3]{
				{"Image Files", "jpeg,jpg,png"},
				{"JPEG Files", "jpeg,jpg"},
				{"PNG Files", "png"}
			};
		default:
			return nullptr;
		}
	}

	bool FileDialog::openFile(std::string& name, FileType type)
	{
		nfdchar_t* outPath;
		nfdfilteritem_t* filters = getDialogFilters(type);
		nfdresult_t result = NFD_OpenDialog(&outPath, filters, getFilterCount(type), nullptr);
		if (result == NFD_OKAY)
		{
			name = outPath;
			NFD_FreePath(outPath);

			return true;
		}

		return false;
	}

	bool FileDialog::saveFile(std::string& name, FileType type)
	{
		nfdchar_t* outPath;
		nfdfilteritem_t* filters = getDialogFilters(type);
		nfdresult_t result = NFD_SaveDialog(&outPath, filters, getFilterCount(type), nullptr, nullptr);
		if (result == NFD_OKAY)
		{
			name = outPath;
			NFD_FreePath(outPath);

			return true;
		}

		return false;
	}
}