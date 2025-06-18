#include "Application.h"
#include "IO.h"
#include <iostream>

namespace mmw = MikuMikuWorld;
mmw::Application app;

int main()
{
	auto args = Platform::GetCommandLineArgs();
	if (args.empty())
	{
		IO::messageBox(APP_NAME, "CommandLineToArgvW failed...", IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error);
		return 1;
	}

	try
	{
		std::string dir = IO::File::getFilepath(args[0]);;
		std::string resDir = Platform::GetResourcePath(dir);
		mmw::Result result = app.initialize(dir, resDir);
		
		if (!result.isOk())
			throw (std::runtime_error(result.getMessage().c_str()));

		for (int i = 1; i < args.size(); ++i)
			app.appendOpenFile(args[i]);

		app.handlePendingOpenFiles();
		app.run();
	}
	catch (const std::exception& ex)
	{
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

#ifdef MMW_WINDOWS
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

			return 0;
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
		return CallWindowProcW((WNDPROC)glfwGetWindowUserPointer(app.getGlfwWindow()), hwnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
#elif defined(MMW_LINUX)
GLFWdropfun MikuMikuWorld::defaultDropFun = NULL;
void MikuMikuWorld::windowDropCallback(GLFWwindow *window, int path_count, const char *paths[])
{
	for (int i = 0; i < path_count && path_count > 0; i++) {
		std::string path = paths[i];
		if (IO::File::exists(path))
			app.appendOpenFile(path);
	}
	if (defaultDropFun)
		defaultDropFun(window, path_count, paths);
}
#endif