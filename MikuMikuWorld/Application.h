#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX

#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "ScoreEditor.h"
#include "ImGuiManager.h"
#include <Windows.h>

LRESULT CALLBACK wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
		Vector2 position;
		Vector2 size;
		UINT_PTR windowTimerId{};
	};

	class Application
	{
	private:
		GLFWwindow* window;
		HWND hwnd;
		std::unique_ptr<ScoreEditor> editor;
		std::unique_ptr<ImGuiManager> imgui;

		UnsavedChangesDialog unsavedChangesDialog;

		float lastFrame;
		float frameDelta;
		bool initialized;
		bool shouldPickScore;
		std::string language;

		std::vector<std::string> pendingOpenFiles;
		std::string pendingDropScoreFile;

		static std::string version;
		static std::string appDir;

		Result initOpenGL();
		void installCallbacks();

		std::string getVersion();

	public:
		static WindowState windowState;

		Application();

		Result initialize(const std::string& root);
		void run();
		void update();
		void frameTime();
		void appendOpenFile(std::string filename);
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void loadResources();
		void dispose();

		GLFWwindow* getGlfwWindow() { return window; }
		HWND& getWindowHandle() { return hwnd; }

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}