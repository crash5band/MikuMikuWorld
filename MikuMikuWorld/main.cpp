#include "Application.h"
#include "UI.h"
#include <iostream>
#include <Windows.h>

namespace mmw = MikuMikuWorld;

int main(int argc, char** argv)
{
	try
	{
		std::string dir = mmw::File::getFilepath(argv[0]);
		mmw::Application app(dir);

		for (int i = 1; i < argc; ++i)
			mmw::Application::pendingOpenFiles.push_back(argv[i]);
		
		app.handlePendingOpenFiles();
		app.run();
	}
	catch (const std::exception& ex)
	{
		std::string msg = "An unhandled exception has occured and the application will now close.\n";
		msg.append(ex.what());
		MessageBox(NULL, msg.c_str(), APP_NAME, MB_OK | MB_ICONERROR);
	}

	return 0;
}