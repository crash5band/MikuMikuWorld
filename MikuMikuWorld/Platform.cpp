#include "Platform.h"

#ifdef MMW_WINDOWS
#include "IO.h"
#include "File.h"

void Platform::OpenUrl(const std::string &url)
{
    ShellExecuteW(0, 0, IO::mbToWideStr(url).c_str(), 0, 0, SW_SHOW);
}

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

std::string Platform::GetCurrentLanguageCode()
{
    LPWSTR lpLocalName = new WCHAR[LOCALE_NAME_MAX_LENGTH];
    int result = GetUserDefaultLocaleName(lpLocalName, LOCALE_NAME_MAX_LENGTH);

    std::wstring wL = lpLocalName;
    wL = wL.substr(0, wL.find_first_of(L"-"));

    delete[] lpLocalName;
    return IO::wideStringToMb(wL);
}

std::string Platform::GetBuildVersion()
{
    wchar_t filename[1024];
    GetModuleFileNameW(NULL, filename, sizeof(filename) / sizeof(*filename));

    DWORD  verHandle = 0;
    UINT   size = 0;
    LPBYTE lpBuffer = NULL;
    DWORD  verSize = GetFileVersionInfoSizeW(filename, NULL);

    int major = 0, minor = 0, build = 0, rev = 0;
    if (verSize != NULL)
    {
        LPSTR verData = new char[verSize];

        if (GetFileVersionInfoW(filename, verHandle, verSize, verData))
        {
            if (VerQueryValue(verData, "\\", (VOID FAR * FAR*) & lpBuffer, &size))
            {
                if (size)
                {
                    VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
                    if (verInfo->dwSignature == 0xfeef04bd)
                    {
                        major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
                        minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
                        rev = (verInfo->dwFileVersionLS >> 16) & 0xffff;
                    }
                }
            }
        }
        delete[] verData;
    }

    return IO::formatString("%d.%d.%d", major, minor, rev);
}

std::vector<std::string> Platform::GetCommandLineArgs()
{
    int argc;
    LPWSTR* args;
    args = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> cmdArgs;
    cmdArgs.reserve((size_t)argc);
    std::transform(args, args + argc, std::back_inserter(cmdArgs), IO::wideStringToMb);
    return cmdArgs;
}

std::string Platform::GetConfigPath(const std::string &app_root)
{
    return IO::File::getFilepath(app_root);
}

std::string Platform::GetResourcePath(const std::string& app_root)
{
    return IO::File::pathConcat(IO::File::getFilepath(app_root), "res", "");
}

#endif

#ifdef MMW_LINUX
#include <fcntl.h>
#include <sys/wait.h>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include "IO.h"
#include "File.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <X11/Xatom.h>

#undef None // X11 macro

#ifndef DEB_PACKAGE_NAME
#error "Missing package name. Make sure your cmake configuration is correct"
#endif

void Platform::OpenUrl(const std::string &url)
{
    std::stringstream command;
    command << "xdg-open " << std::quoted(url) << " 2> /dev/null";
    system(command.str().c_str());
}

FILE *Platform::OpenFile(const std::string &filename, const std::string &mode)
{
    return fopen(filename.c_str(), mode.c_str());
}

// Create a process with the specified arguments
// Return true if successful
// Optionally direct the child stdout to a pipe
// Optionally return the id of the child process
static bool StartProcess(const char* file, char* const* args, int* outfd = nullptr, pid_t* child = nullptr) {
    union {
        int fd[2];
        struct { int read; int write; };
    } channels;
    if (pipe(channels.fd) != 0)
        return false;

    pid_t p = vfork();
    if (p < 0) 
        return false;
    if (p == 0) {
        // Child process
        int null = open("/dev/null", O_WRONLY);
        
        dup2(channels.write, STDOUT_FILENO);
        dup2(null,           STDERR_FILENO);

        close(null);
        close(channels.read);
        close(channels.write);
        
        execvp(file, args);
        _exit(-1);
    }
    else {
        // Parent process
        close(channels.write);
        if (outfd)
            *outfd = channels.read;
        else
            close(channels.read);
        if (child)
            *child = p;
        return true;
    }
}

// Called by ZenityOpen to set the opened dialog window as child of the current window (if possible)
// Note that this only works on X11. There is no way to do this on Wayland
static void SetWindowTransient(pid_t target, int& stat) {
    struct find_xwindow_with_pid
    {
        pid_t pid;
        Display* display;
        std::vector<Window> result;
        Atom pid_atom;

        find_xwindow_with_pid(pid_t pid, Display* display) : pid{pid}, display{display}, result{} {
            pid_atom = XInternAtom(display, "_NET_WM_PID", True);
            if (pid_atom == 0)
                throw std::runtime_error("find_xwindow_with_pid: _NET_WM_PID atom not found!");
        }

        std::vector<Window>& operator()(Window current) {
            Atom           type;
            int            format;
            unsigned long  nItems;
            unsigned long  bytesAfter;
            unsigned char *propPID = 0;
            XGetWindowProperty(display, current, pid_atom, 0, 1, False, XA_CARDINAL, &type, &format, &nItems, &bytesAfter, &propPID);
            if(propPID != 0 && type == XA_CARDINAL && pid == *reinterpret_cast<pid_t*>(propPID))
                result.emplace_back(current);
            
            Window  wRoot;
            Window  wParent;
            Window* wChild;
            unsigned  nChildren;
            if (XQueryTree(display, current, &wRoot, &wParent, &wChild, &nChildren)) {
                for (unsigned i = 0; i < nChildren; i++) {
                    this->operator()(wChild[i]);
                }
            }

            if (wChild)
                XFree(wChild);

            if (propPID)
                XFree(propPID);
            return this->result;
        }
    };
    Display* display = glfwGetX11Display();
    if (display == NULL) {
        glfwGetError(nullptr); // clear the error
        return;
    }
    GLFWwindow* gwindow = glfwGetCurrentContext();
    if (gwindow == NULL) 
        return;
    Window parent_window = glfwGetX11Window(gwindow);
    find_xwindow_with_pid window_finder(target, display);

    Atom NET_WM_STATE = XInternAtom(display, "_NET_WM_STATE", False);
    Atom STATE_NO_MODAL = XInternAtom(display, "_NET_WM_STATE_MODAL", False);

    bool timer_started = false;
    using timer_clock_t = std::chrono::steady_clock;
    std::chrono::time_point<timer_clock_t> start;

    while (waitpid(target, &stat, WNOHANG) == 0) {
        std::vector<Window>& target_windows = window_finder(XDefaultRootWindow(display));
        if (!target_windows.empty()) {
            for (auto& window : target_windows) {
                Atom           type;
                int            format;
                unsigned long  nItems;
                unsigned long  bytesAfter;
                Atom *         props = 0;
                XGetWindowProperty(display, window, NET_WM_STATE, 0, 1, False, XA_ATOM, &type, &format, &nItems, &bytesAfter, (unsigned char**)&props);

                bool has_modal_flag = false;
                if(props && type == XA_ATOM) {
                    for (unsigned long i = 0; i < nItems; i++)
                        if (props[i] == STATE_NO_MODAL) {
                            has_modal_flag = true;
                            break;
                        }
                }
                if (props)
                    XFree(props);
                
                if (has_modal_flag) {
                    XSetTransientForHint(display, window, parent_window);
                    Atom NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
                    Atom WINDOW_TYPE_DIALOG = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
                    Atom STATE_NO_TASKBAR = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
                    XChangeProperty(display, window, NET_WM_WINDOW_TYPE, XA_ATOM, 32, PropModeReplace, (unsigned char*)&WINDOW_TYPE_DIALOG, 1);
                    XChangeProperty(display, window, NET_WM_STATE, XA_ATOM, 32, PropModeAppend, (unsigned char*)&STATE_NO_TASKBAR, 1);
                    XMapWindow(display, window);
                    XFlush(display);
                    return;
                }
            }
            // No modal window found? Maybe zenity is still initializing. Try waiting 10s
            if (!timer_started) {
                start = timer_clock_t::now();
                timer_started = true;
            }
            else {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(timer_clock_t::now() - start).count();
                if (duration > 10)
                    break;
            }
            target_windows.clear();
        }  
    }
}

static bool ZenityOpen(const std::vector<std::string>& arguments, std::string& output, int* exit_code = nullptr) {
    char buffer[256];
    char zenity[] = "zenity", modal[] = "--modal";
    std::vector<char*> args = { zenity };
    std::transform(arguments.begin(), arguments.end(), std::back_inserter(args), [](const std::string& s) { return const_cast<char*>(s.data()); });
    args.push_back(modal);
    args.push_back(NULL);
    int outfd, stat = 0;
    pid_t pid;
    if(!StartProcess(args[0], args.data(), &outfd, &pid))
        return false;

    SetWindowTransient(pid, stat);

    if (stat == 0)
        waitpid(pid, &stat, 0);

    ssize_t nread;
    while((nread = read(outfd, buffer, sizeof(buffer))) > 0)
        output.append(buffer, nread);
    close(outfd);

    if (nread < 0)
        return false;
    
    if (exit_code) {
        if (WIFEXITED(stat))
            *exit_code = WEXITSTATUS(stat);
        else
            *exit_code = -1;
    }
    return true;
}

IO::FileDialogResult Platform::OpenFileDialog(IO::DialogType type, IO::DialogSelectType selectType, IO::FileDialog &dialogOptions)
{
    std::vector<std::string> arguments;
    arguments.push_back("--file-selection");
    if (!dialogOptions.title.empty())
        arguments.push_back("--title=" + dialogOptions.title);
    if (type == IO::DialogType::Save)
        arguments.push_back("--save");
    if (!dialogOptions.inputFilename.empty())
        arguments.push_back("--filename=" + dialogOptions.inputFilename);
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
        arguments.push_back("--file-filter=" + filterValue);
    }
    if (selectType == IO::DialogSelectType::Folder)
        arguments.push_back("--directory");

    std::string& outFile = dialogOptions.outputFilename;
    outFile.clear();
    if (!ZenityOpen(arguments, outFile))
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
    std::vector<std::string> arguments;
    bool has_custom_button = false;
    switch (icon)
    {
    default:
    case IO::MessageBoxIcon::None:
    case IO::MessageBoxIcon::Information:
        if (buttons == IO::MessageBoxButtons::Ok)
            arguments.push_back("--info");
        else {
            arguments.push_back("--question");
            arguments.push_back("--icon=dialog-information");
            has_custom_button = true;
        }
        break;
    case IO::MessageBoxIcon::Warning:
        if (buttons == IO::MessageBoxButtons::Ok)
            arguments.push_back("--warning");
        else {
            arguments.push_back("--question");
            arguments.push_back("--icon=dialog-warning");
            has_custom_button = true;
        }
        break;
    case IO::MessageBoxIcon::Error:
        if (buttons == IO::MessageBoxButtons::Ok)
            arguments.push_back("--error");
        else {
            arguments.push_back("--question");
            arguments.push_back("--icon=dialog-error");
            has_custom_button = true;
        }
        break;
    case IO::MessageBoxIcon::Question:
        arguments.push_back("--question");
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
            arguments.push_back(std::string("--ok-label=") + MESSAGE_BOX_RESPONSES[labels[0]]);
        if (labels[1] != None)
            arguments.push_back(std::string("--cancel-label=") + MESSAGE_BOX_RESPONSES[labels[1]]);
        if (labels[2] != None)
            arguments.push_back(std::string("--extra-button=") + MESSAGE_BOX_RESPONSES[labels[2]]);
    }
    if (!title.empty())
        arguments.push_back("--title=" + title);
    arguments.push_back("--text=" + message);

    std::string output;
    int exit_code;
    ZenityOpen(arguments, output, &exit_code);
    switch (exit_code) {
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
    return IO::MessageBoxResult::None;
}

// Fallback of GetCurrentLanguageCode using C/C++ locale library to determine the language
static std::string GetLocaleCode() {
    // Locale string is either a locale name in XPG format; or a list of catergory=name seperate by ';'
    std::string locale_string = std::locale("").name();
    if (locale_string == "*")
        // Unnamed locale
        return {};
    size_t pos = locale_string.find("LC_CTYPE=");
    if (pos != std::string::npos) {
        pos += 9;
        size_t end_pos = locale_string.find(';', pos);
        if (end_pos != std::string::npos)
            locale_string = locale_string.substr(pos, end_pos - pos);
        else
            locale_string = locale_string.substr(pos);
    }
    // Extract the language code
    // language[_territory[.codeset]][@modifier]
    pos = locale_string.find_first_of("_.@");
    if (pos != std::string::npos)
        locale_string.erase(pos);
    return locale_string;
}

// Using linux environment to retrieve the code
static std::string GetLanguageCode() {
    const char* lang_env = getenv("LANGUAGE");
    if (!lang_env)
        return {};
    std::string languages = lang_env;
    // Extract the language code
    // language[_territory[.codeset]][@modifier]
    size_t pos = languages.find_first_of("_.@");
    while (pos != std::string::npos) {
        size_t end_pos = languages.find(':', pos);
        if (end_pos != std::string::npos)
            languages.erase(pos, end_pos - pos);
        else
            languages.erase(pos);
        pos = languages.find_first_of("_.@", pos);
    }
    return languages;
}

std::string Platform::GetCurrentLanguageCode()
{
    std::string code = GetLanguageCode();
    return !code.empty() ? code : GetLocaleCode();
}

std::string Platform::GetBuildVersion()
{
#ifdef MMW_APP_VERSION
    return MMW_APP_VERSION;
#else
#error "Missing version info. Check CMakeLists.txt if the definition is set."
#endif
}

std::vector<std::string> Platform::GetCommandLineArgs()
{
    std::vector<std::string> cmdargs;
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary | std::ios::in);
    if (cmdline.is_open()) {
        for (std::string arg; std::getline(cmdline, arg, '\0'); )
            cmdargs.push_back(arg);
    }
    return cmdargs;
}

std::string Platform::GetConfigPath(const std::string &app_root)
{
    std::string config_dir;
    const char* env_dir;;
    // Optional override
    if (env_dir = getenv("MMW_CONFIG_HOME"))
        config_dir = IO::File::pathConcat(env_dir, "");
    else
    if (env_dir = getenv("XDG_CONFIG_HOME"))
        config_dir = IO::File::pathConcat(env_dir, DEB_PACKAGE_NAME, "");
    else
    if (env_dir = getenv("HOME")) 
        config_dir = IO::File::pathConcat(env_dir, ".config", DEB_PACKAGE_NAME, "");

    if (!std::filesystem::exists(config_dir))
        std::filesystem::create_directories(config_dir);
    return config_dir;
}

std::string Platform::GetResourcePath(const std::string& app_root)
{
    std::string res_dir = IO::File::pathConcat(IO::File::getFilepath(app_root), "res", "");
    if (IO::File::exists(res_dir))
        return res_dir;

    // Checking environment path
    const char* env_dir = getenv("XDG_DATA_DIRS");
    if (env_dir && *env_dir != '\0') {
        std::string_view data_dir_str = env_dir;
        for (size_t pos = 0, end_pos = 0; end_pos != std::string_view::npos; pos = end_pos + 1) {
            end_pos = data_dir_str.find(':', pos + 1);
            std::filesystem::path data_dir = data_dir_str.substr(pos, end_pos - pos);
            data_dir /= DEB_PACKAGE_NAME;
            data_dir /= "";
            if (std::filesystem::exists(data_dir))
                return data_dir.string();
        }
    }
    
    // Fallback to package path
    return "/usr/share/" DEB_PACKAGE_NAME "/";
}

#endif
