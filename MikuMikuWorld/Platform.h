#pragma once
#ifdef _WIN32

#define MMW_WINDOWS
#include <Windows.h>
#elif defined(__APPLE__)

#define MMW_MACOS
#import <string>
#import <vector>
#elif defined(__unix__) || defined(__unix)

#define MMW_LINUX
#include "unistd.h"
#include <cstdio>
#include <stdint.h>
#include <string>
#include <vector>

#endif

namespace IO {
    enum class FileDialogResult : uint8_t;
    enum class DialogType : uint8_t;
    enum class DialogSelectType : uint8_t;
    struct FileDialogFilter;
    class FileDialog;

    enum class MessageBoxButtons : uint8_t;
    enum class MessageBoxIcon : uint8_t;
    enum class MessageBoxResult : uint8_t;
}

namespace Platform {
    void OpenUrl(const std::string& url);
    IO::FileDialogResult OpenFileDialog(IO::DialogType type, IO::DialogSelectType selectType, IO::FileDialog& dialogOptions);
    IO::MessageBoxResult OpenMessageBox(const std::string& title, const std::string& message, IO::MessageBoxButtons buttons, IO::MessageBoxIcon icon, void* parentWindow = nullptr);
    std::string GetCurrentLanguageCode();
    std::string GetBuildVersion();
    FILE* OpenFile(const std::string& filename, const std::string& mode);
    std::vector<std::string> GetCommandLineArgs();
    std::string GetResourcePath(const std::string& root);
}