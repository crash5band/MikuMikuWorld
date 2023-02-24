#include "FileDialog.h"
#include "StringOperations.h"

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include "GLFW/glfw3native.h"

#include <Windows.h>

namespace MikuMikuWorld
{
	const wchar_t* FileDialog::getExtensionFromType(FileType type)
	{
		switch (type)
		{
		case MikuMikuWorld::FileType::MMWSFile:
			return L".mmws";
		case MikuMikuWorld::FileType::SUSFile:
			return L".sus";
		default:
			return L"";
		}
	}

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

	const wchar_t* FileDialog::getDialogFilters(FileType type)
	{
		switch (type)
		{
		case FileType::ScoreFile:
			return L"Score Files (*.mmws;*.sus)\0*.mmws;*.sus\0MikuMikuWorld Score (*.mmws)\0*.mmws\0Sliding Universal Score(*.sus)\0*.sus\0All Files(*.*)\0*.*\0";
		case FileType::MMWSFile:
			return L"MikuMikuWorld Score (.mmws)\0*.mmws";
		case FileType::SUSFile:
			return L"Sliding Universal Score(.sus)\0 *.sus";
		case FileType::AudioFile:
			return L"Audio Files(*.mp3;*.wav;*.flac;*.ogg)\0*.mp3;*.wav;*.flac;*.ogg\0MP3 Files(*.mp3)\0*.mp3\0WAV Files(*.wav)\0*.wav\0FLAC Files(*.flac)\0*.flac\0OGG Vorbis Files(*.ogg)\0*.ogg\0";
		case FileType::ImageFile:
			return L"Image Files (*.jpeg;*.jpg;*.png)\0*.jpeg;*.jpg;*.png\0JPEG Files(*.jpeg;*.jpg)\0*.jpeg;*.jpg\0PNG Files(*.png)\0*.png\0";
		default:
			return L"All Files(*.*)\0*.*\0";
		}
	}

	bool FileDialog::openFile(std::string& name, FileType type)
	{
		wchar_t filename[1024];
		filename[0] = '\0';

		std::wstring title{ L"Open " };
		title.append(getDialogTitle(type));

		HWND ownerHandle = NULL;
		GLFWwindow* owner = glfwGetCurrentContext();
		if (owner)
		{
			ownerHandle = glfwGetWin32Window(owner);
		}

		OPENFILENAMEW ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.hwndOwner = ownerHandle;
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = getDialogFilters(type);
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = title.c_str();
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER
			| OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameW(&ofn))
		{
			name = wideStringToMb(ofn.lpstrFile);
			return true;
		}

		return false;
	}

	bool FileDialog::saveFile(std::string& name, FileType type)
	{
		wchar_t filename[1024];
		filename[0] = L'\0';

		if (name.size())
		{
			std::wstring wFilename = mbToWideStr(name);
			lstrcpynW(filename, wFilename.c_str(), 1024);
		}

		std::wstring title{ L"Save " };
		title.append(getDialogTitle(type));

		HWND ownerHandle = NULL;
		GLFWwindow* owner = glfwGetCurrentContext();
		if (owner)
		{
			ownerHandle = glfwGetWin32Window(owner);
		}

		OPENFILENAMEW ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.hwndOwner = ownerHandle;
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFilter = getDialogFilters(type);
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrTitle = title.c_str();
		ofn.lpstrDefExt = getExtensionFromType(type);
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_EXPLORER
			| OFN_HIDEREADONLY | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;

		if (GetSaveFileNameW(&ofn))
		{
			name = wideStringToMb(ofn.lpstrFile);
			return true;
		}

		return false;
	}
}