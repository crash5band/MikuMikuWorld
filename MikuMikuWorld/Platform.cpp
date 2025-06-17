#include "Platform.h"

#ifdef MMW_WINDOWS
#include "IO.h"
#include "File.h"

FILE *Platform::OpenFile(const std::string &filename, const std::string &mode)
{
    return _wfopen(IO::mbToWideStr(filename).c_str(), IO::mbToWideStr(mode).c_str());
}

IO::FileDialogResult Platform::OpenFileDialog(IO::DialogType type, IO::DialogSelectType selectType, IO::FileDialog &dialogOptions)
{
    std::wstring wTitle = IO::mbToWideStr(dialogOptions.title);

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = reinterpret_cast<HWND>(dialogOptions.parentWindowHandle);
    ofn.lpstrTitle = wTitle.c_str();
    ofn.nFilterIndex = dialogOptions.filterIndex + 1;
    ofn.nFileOffset = 0;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    std::wstring wDefaultExtension = IO::mbToWideStr(dialogOptions.defaultExtension);
    ofn.lpstrDefExt = wDefaultExtension.c_str();

    std::vector<std::wstring> ofnFilters;
    ofnFilters.reserve(dialogOptions.filters.size());

    /*
        since '\0' terminates the string,
        we'll do a C# by using ' | ' then replacing it with '\0' when constructing the final wide string
    */
    std::string filtersCombined;
    for (const auto &filter : dialogOptions.filters)
    {
        filtersCombined
            .append(filter.filterName)
            .append(" (")
            .append(filter.filterType)
            .append(")|")
            .append(filter.filterType)
            .append("|");
    }

    std::wstring wFiltersCombined = IO::mbToWideStr(filtersCombined);
    std::replace(wFiltersCombined.begin(), wFiltersCombined.end(), '|', '\0');
    ofn.lpstrFilter = wFiltersCombined.c_str();

    std::wstring wInputFilename = IO::mbToWideStr(dialogOptions.inputFilename);
    wchar_t ofnFilename[1024]{0};

    // suppress return value not used warning
#pragma warning(suppress : 6031)
    lstrcpynW(ofnFilename, wInputFilename.c_str(), 1024);
    ofn.lpstrFile = ofnFilename;

    if (type == IO::DialogType::Save)
    {
        ofn.Flags |= OFN_HIDEREADONLY;
        if (GetSaveFileNameW(&ofn))
        {
            dialogOptions.outputFilename = IO::wideStringToMb(ofn.lpstrFile);
        }
        else
        {
            // user canceled
            return IO::FileDialogResult::Cancel;
        }
    }
    else if (GetOpenFileNameW(&ofn))
    {
        dialogOptions.outputFilename = IO::wideStringToMb(ofn.lpstrFile);
    }
    else
    {
        return IO::FileDialogResult::Cancel;
    }

    if (dialogOptions.outputFilename.empty())
        return IO::FileDialogResult::Cancel;

    dialogOptions.filterIndex = ofn.nFilterIndex - 1;
    return IO::FileDialogResult::OK;
}

IO::MessageBoxResult Platform::OpenMessageBox(const std::string &title, const std::string &message, IO::MessageBoxButtons buttons, IO::MessageBoxIcon icon, void *parentWindow)
{
    UINT flags = 0;
    switch (icon)
    {
    case IO::MessageBoxIcon::Information:
        flags |= MB_ICONINFORMATION;
        break;
    case IO::MessageBoxIcon::Warning:
        flags |= MB_ICONWARNING;
        break;
    case IO::MessageBoxIcon::Error:
        flags |= MB_ICONERROR;
        break;
    case IO::MessageBoxIcon::Question:
        flags |= MB_ICONQUESTION;
        break;
    default:
        break;
    }

    switch (buttons)
    {
    case IO::MessageBoxButtons::Ok:
        flags |= MB_OK;
        break;
    case IO::MessageBoxButtons::OkCancel:
        flags |= MB_OKCANCEL;
        break;
    case IO::MessageBoxButtons::YesNo:
        flags |= MB_YESNO;
        break;
    case IO::MessageBoxButtons::YesNoCancel:
        flags |= MB_YESNOCANCEL;
        break;
    default:
        break;
    }

    const int result = MessageBoxExW(reinterpret_cast<HWND>(parentWindow), IO::mbToWideStr(message).c_str(), IO::mbToWideStr(title).c_str(), flags, 0);
    switch (result)
    {
    case IDABORT:
        return IO::MessageBoxResult::Abort;
    case IDCANCEL:
        return IO::MessageBoxResult::Cancel;
    case IDIGNORE:
        return IO::MessageBoxResult::Ignore;
    case IDNO:
        return IO::MessageBoxResult::No;
    case IDYES:
        return IO::MessageBoxResult::Yes;
    case IDOK:
        return IO::MessageBoxResult::Ok;
    default:
        return IO::MessageBoxResult::None;
    }
}

#endif

#ifdef MMW_LINUX
#include <fcntl.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <GLFW/glfw3.h>
#include "IO.h"
#include "File.h"

FILE *Platform::OpenFile(const std::string &filename, const std::string &mode)
{
    return fopen(filename.c_str(), mode.c_str());
}

static int OpenProcess(const std::string& command, std::string& output) {
    GLFWwindow* window = glfwGetCurrentContext();
    bool glfwInitialized = glfwGetError(NULL) == GLFW_NO_ERROR;
    char buffer[256];
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) return -1;
    int pfd = fileno(pipe);
    fcntl(pfd, F_SETFL, O_NONBLOCK);
    while (true)
    {
        ssize_t nread = read(pfd, buffer, sizeof(buffer));
        if (nread < 0)
        {
            if (errno == EAGAIN)
            {
                // To prevent window from freezing we need to let glfw process events
                if (glfwInitialized)
                    glfwPollEvents();
            }
            else
            {
                pclose(pipe);
                return -1;
            }
        }
        else if (nread == 0)
        {
            return pclose(pipe);
        }
        else
        {
            output.append(buffer, nread);
        }
    }
}

IO::FileDialogResult Platform::OpenFileDialog(IO::DialogType type, IO::DialogSelectType selectType, IO::FileDialog &dialogOptions)
{
    std::stringstream command;
    command << "zenity --file-selection";
    if (!dialogOptions.title.empty())
        command << " --title=" << std::quoted(dialogOptions.title);
    if (type == IO::DialogType::Save)
        command << " --save";
    if (!dialogOptions.inputFilename.empty())
        command << " --filename=" << std::quoted(dialogOptions.inputFilename);
    for (auto const &filter : dialogOptions.filters)
    {
        // Converting to the format "NAME | PATTERN1 PATTERN2 ..."
        std::string filterValue = filter.filterName;
        filterValue.append(" | ");
        // Replace ; with empty string
        std::transform(
            filter.filterType.begin(),
            filter.filterType.end(),
            std::back_inserter(filterValue),
            [](const char &c) { return c == ';' ? ' ' : c; }
        );
        command << " --file-filter=" << std::quoted(filterValue);
    }
    if (selectType == IO::DialogSelectType::Folder)
        command << " --directory";
    command << " 2>/dev/null";

    std::string& outFile = dialogOptions.outputFilename;
    outFile.clear();
    if (OpenProcess(command.str(), outFile) == -1)
        return IO::FileDialogResult::Error;
    if (outFile.empty())
        return IO::FileDialogResult::Cancel;

    auto pos = outFile.find('\n');
    if (pos == std::string::npos)
        return IO::FileDialogResult::Error;
    outFile.erase(pos);

    std::filesystem::path result_path(outFile);
    if (!result_path.has_extension() && !dialogOptions.defaultExtension.empty())
    {
        result_path.replace_extension(dialogOptions.defaultExtension);
        outFile = result_path;
    }

    return IO::FileDialogResult::OK;
}

IO::MessageBoxResult Platform::OpenMessageBox(const std::string &title, const std::string &message, IO::MessageBoxButtons buttons, IO::MessageBoxIcon icon, void *parentWindow)
{
    std::stringstream command;
    command << "zenity";
    bool has_custom_button = false;
    switch (icon)
    {
    default:
    case IO::MessageBoxIcon::None:
    case IO::MessageBoxIcon::Information:
        if (buttons == IO::MessageBoxButtons::Ok)
            command << " --info";
        else {
            command << " --question --icon=dialog-information";
            has_custom_button = true;
        }
        break;
    case IO::MessageBoxIcon::Warning:
        if (buttons == IO::MessageBoxButtons::Ok)
            command << " --warning";
        else {
            command << " --question --icon=dialog-warning";
            has_custom_button = true;
        }
        break;
    case IO::MessageBoxIcon::Error:
        if (buttons == IO::MessageBoxButtons::Ok)
            command << " --error";
        else {
            command << " --question --icon=dialog-error";
            has_custom_button = true;
        }
        break;
    case IO::MessageBoxIcon::Question:
        command << " --question";
        has_custom_button = buttons != IO::MessageBoxButtons::YesNo;
        break;
    }
    const char* MESSAGE_BOX_RESPONSES[] = {"Ok", "Yes", "No", "Cancel"};
    IO::MessageBoxResult MESSAGE_BOX_RESULT[] = { IO::MessageBoxResult::Ok, IO::MessageBoxResult::Yes, IO::MessageBoxResult::No, IO::MessageBoxResult::Cancel, IO::MessageBoxResult::Ok };
    enum { Ok = 0, Yes, No, Cancel, None } labels[3] = {None, None, None};
    switch (buttons)
    {
    case IO::MessageBoxButtons::Ok:
        labels[0] = Ok;
        break;
    case IO::MessageBoxButtons::OkCancel:
        labels[0] = Ok;
        labels[1] = Cancel;
        break;
    case IO::MessageBoxButtons::YesNo:
        labels[0] = Yes;
        labels[1] = No;
        break;
    case IO::MessageBoxButtons::YesNoCancel:
        labels[0] = Yes;
        labels[1] = No;
        labels[2] = Cancel;
        break;
    }
    if (has_custom_button) {
        if (labels[0] != None)
            command << " --ok-label=" << MESSAGE_BOX_RESPONSES[labels[0]];
        if (labels[1] != None)
            command << " --cancel-label=" << MESSAGE_BOX_RESPONSES[labels[1]];
        if (labels[2] != None)
            command << " --extra-button=" << MESSAGE_BOX_RESPONSES[labels[2]];
    }
    if (!title.empty())
        command << " --title=" << std::quoted(title);
    command << " --text=" << std::quoted(message);
    command << " 2>/dev/null";

    std::string output;
    int status = OpenProcess(command.str(), output);
    if (status != -1 && WIFEXITED(status)) {
        switch (WEXITSTATUS(status)) {
            default: return IO::MessageBoxResult::None;
            case 0: return MESSAGE_BOX_RESULT[labels[0]];
            case 1:
                auto pos = output.find('\n');
                if (pos == std::string::npos)
                    return MESSAGE_BOX_RESULT[labels[1]];
                output.erase(pos);
                ssize_t index = std::find_if(MESSAGE_BOX_RESPONSES + 0, MESSAGE_BOX_RESPONSES + 4, [&output](const char*& s) { return output == s; }) - MESSAGE_BOX_RESPONSES;
                return MESSAGE_BOX_RESULT[index];
                
        }
    }
    return IO::MessageBoxResult::None;
}
#endif
