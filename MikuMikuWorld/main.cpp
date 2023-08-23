#include "Application.h"
#include "IO.h"
#include "UI.h"
#include <iostream>
#include <Windows.h>

namespace mmw = MikuMikuWorld;
mmw::Application app;

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
	app = mmw::Application(dir);

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

LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TIMER:
		if (mmw::Application::windowState.windowDragging && wParam == mmw::Application::windowState.windowTimerId)
		{
			// grabbing the glfw window blocks the message queue causing the application to stop rendering
			// so we handle the message ourselves and update the UI explicitly
			if (app.getGlfwWindow())
				app.update();
		}
		break;

	case WM_ENTERSIZEMOVE:
		mmw::Application::windowState.windowDragging = true;
		break;

	case WM_EXITSIZEMOVE:
		mmw::Application::windowState.windowDragging = false;
		break;

	case WM_DROPFILES:
		if (HDROP dropHandle = reinterpret_cast<HDROP>(wParam); dropHandle != NULL)
		{
			const UINT filesCount = ::DragQueryFileW(dropHandle, 0xFFFFFFFF, NULL, 0u);
			for (UINT i = 0; i < filesCount; ++i)
			{
				const UINT bufferSize = ::DragQueryFileW(dropHandle, i, NULL, 0u);
				if (bufferSize > 0)
				{
					std::wstring wFilename(bufferSize + 1, 0);
					if (::DragQueryFileW(dropHandle, i, wFilename.data(), static_cast<UINT>(wFilename.size())) != 0)
						app.appendOpenFile(IO::wideStringToMb(wFilename.data()));
				}
			}

			::DragFinish(dropHandle);
		}
		break;

	default:
		// we don't handle this message ourselves so delegate it to the original glfw window's proc
		return CallWindowProc(
			(WNDPROC)glfwGetWindowUserPointer(app.getGlfwWindow()),
			app.getWindowHandle(),
			uMsg,
			wParam,
			lParam
		);
	}

	return 0;
}