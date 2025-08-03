#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "Platform.h"
#if defined (MMW_WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(MMW_MACOS)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#define GLFW_EXPOSE_NATIVE_EGL
#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "ScoreEditor.h"
#include "ImGuiManager.h"

namespace MikuMikuWorld
{
	class Result;

	struct WindowState
	{
		bool resetting = false;
		bool vsync = true;
		bool showPerformanceMetrics = false;
		bool maximized = false;
		bool closing = false;
		bool shouldPickScore = false;
		bool dragDropHandled = true;
		bool windowDragging = false;
		float lastDpiScale = 0.0f;
		void* windowHandle{ 0 };
		Vector2 position{};
		Vector2 size{};
#ifdef MMW_WINDOWS
		UINT_PTR windowTimerId{};
#endif
	};

	class Application
	{
	private:
		GLFWwindow* window{ nullptr };
		std::unique_ptr<ScoreEditor> editor;
		std::unique_ptr<ImGuiManager> imgui;
		UnsavedChangesDialog unsavedChangesDialog;

		bool initialized{ false };
		bool shouldPickScore{ false };
		std::string language;

		std::vector<std::string> pendingOpenFiles;

		static std::string version;
		static std::string appDir;
		static std::string resDir;

		Result initOpenGL();
		std::string getVersion();

	public:
		static WindowState windowState;
		static std::string pendingLoadScoreFile;

		Application();

		Result initialize(const std::string& root, const std::string& res);
		void run();
		void update();
		void appendOpenFile(const std::string& filename);
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void loadResources();
		void dispose();
		bool attemptSave();
		bool isEditorUpToDate() const;

		GLFWwindow* getGlfwWindow() { return window; }

		static const std::string& getAppDir();
		static const std::string& getResDir();
		static const std::string& getAppVersion();
	};
}

#ifdef MMW_WINDOWS
LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#elif defined(MMW_LINUX)
namespace MikuMikuWorld {
	extern GLFWdropfun defaultDropFun;
	void windowDropCallback(GLFWwindow* window, int path_count, const char* paths[]);
}
#endif