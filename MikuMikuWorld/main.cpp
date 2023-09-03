#include "Application.h"
#include "IO.h"
#include "UI.h"
#include <iostream>
#include <Windows.h>

namespace mmw = MikuMikuWorld;

int main()
{
	int argc;
	LPWSTR* args;
	args = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!args)
	{
		IO::messageBox(APP_NAME, "CommandLineToArgvW failed...", IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error);
		return 1;
	}

	std::string dir = IO::File::getFilepath(IO::wideStringToMb(args[0]));
	mmw::Application app(dir);

	try
	{
		mmw::Result result = app.initialize();
		if (!result.isOk())
		{
			IO::messageBox(APP_NAME, result.getMessage(), IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error);
			return 1;
		}

		for (int i = 1; i < argc; ++i)
			app.appendOpenFile(IO::wideStringToMb(args[i]));

		app.handlePendingOpenFiles();
		app.run();
	}
	catch (const std::exception& ex)
	{
		std::string msg = "An unhandled exception has occured and the application will now close.\n";
		msg.append(ex.what());
		IO::messageBox(APP_NAME, msg, IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error);
	}

	app.dispose();
	return 0;
}
