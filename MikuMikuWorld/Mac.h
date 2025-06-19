#import <string>
#import "IO.h"
#import "File.h"

namespace platform {

void openURL(const std::string &url);

IO::MessageBoxResult showMessageBox(const std::string& title, const std::string& message, IO::MessageBoxButtons buttons, IO::MessageBoxIcon icon);

std::string getUserLanguageCode();

IO::FileDialogResult showFileDialog(const std::string& title, const std::string& inputFilename, const std::string& defaultExtension, std::vector<IO::FileDialogFilter> filters, IO::DialogType type, std::string& outputFilename);

std::string getBuildVersion();

}
