#include "Application.h"
#include "StringOperations.h"
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
		printf("main(): CommandLineToArgvW failed\n");
		MessageBox(NULL, "main(): CommandLineToArgvW failed\n", APP_NAME, MB_OK | MB_ICONERROR);

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
		MessageBox(NULL, msg.c_str(), APP_NAME, MB_OK | MB_ICONERROR);
	}

	app.dispose();
	return 0;
}