#include "Application.h"
#include "IO.h"
#include <iostream>

namespace mmw = MikuMikuWorld;
mmw::Application app;

int main(int argc, char* argv[])
{
	try
	{
		std::string homeDirectory = [NSHomeDirectory() UTF8String];
		std::string resourcePath = [[[NSBundle mainBundle] resourcePath] UTF8String];
		std::string dir = homeDirectory + "/MikuMikuWorld/";
		std::string resDir = resourcePath + "/res/";

		std::filesystem::create_directory(dir);

		mmw::Result result = app.initialize(dir, resDir);

		if (!result.isOk())
			throw (std::runtime_error(result.getMessage().c_str()));

		for (int i = 1; i < argc; ++i)
			app.appendOpenFile(argv[i]);

		app.handlePendingOpenFiles();
		app.run();
	}
	catch (const std::exception& ex)
	{
		std::cout << ex.what() << std::endl;
		IO::MessageBoxButtons actions = IO::MessageBoxButtons::Ok;
		std::string msg = "An unhandled exception has occurred and the application will now close.";
		if (!app.isEditorUpToDate())
		{
			msg.append("\nDo you want to save the current score?");
			actions = IO::MessageBoxButtons::YesNo;
		}

		msg
			.append("\n\nError: ")
			.append(ex.what())
			.append("\nApplication Version: ")
			.append(mmw::Application::getAppVersion());
			
		IO::MessageBoxResult result = IO::messageBox(APP_NAME, msg, actions, IO::MessageBoxIcon::Error, mmw::Application::windowState.windowHandle);
		if (!app.isEditorUpToDate() && result == IO::MessageBoxResult::Yes && app.attemptSave())
		{
			IO::messageBox(APP_NAME, "Save successful", IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Information, mmw::Application::windowState.windowHandle);
		}

		app.writeSettings();
	}

	app.dispose();
	return 0;
}
