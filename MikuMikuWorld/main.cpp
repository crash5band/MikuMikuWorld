#include "Application.h"
#include "StringOperations.h"
#include "UI.h"
#include <iostream>
#include <Windows.h>
#include <tinyfiledialogs.h>

namespace mmw = MikuMikuWorld;

int main()
{
	int argc;
	LPWSTR* args;
	args = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!args)
	{
		printf("main(): CommandLineToArgvW failed\n");
		tinyfd_messageBox(APP_NAME, "Could not parse command line arguments", "ok", "error", 1);

		return 1;
	}

	std::string dir = mmw::File::getFilepath(mmw::wideStringToMb(args[0]));
	mmw::Application app(dir);

	try
	{
		app.initialize();
		for (int i = 1; i < argc; ++i)
			mmw::Application::pendingOpenFiles.push_back(mmw::wideStringToMb(args[i]));
		
		app.handlePendingOpenFiles();
		app.run();
	}
	catch (const std::exception& ex)
	{
		std::string msg = "An unhandled exception has occured and the application will now close.\n";
		msg.append(ex.what());
		tinyfd_messageBox(APP_NAME, msg.c_str(), "ok", "error", 1);
	}

	app.dispose();
	return 0;
}