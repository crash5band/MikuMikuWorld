#include "Application.h"
#include <iostream>
#include <Windows.h>

namespace mmw = MikuMikuWorld;

int main(int argc, char** argv)
{
	printf("Welcome to MikuMikuWorld!\n");
	try
	{
		std::string dir = mmw::File::getFilepath(argv[0]);
		mmw::Application app(dir);
		app.run();
	}
	catch (const std::exception& ex)
	{
		std::string msg = "An unhandled exception has occured and the application will now close.\n";
		msg.append(ex.what());
		MessageBox(NULL, msg.c_str(), "MikuMikuWorld", MB_OK | MB_ICONERROR);
	}

	return 0;
}