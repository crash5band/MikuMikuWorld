#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "ImGuiManager.h"
#include "ScoreEditor.h"
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
		void* windowHandle;
		Vector2 position{};
		Vector2 size{};
		UINT_PTR windowTimerId{};
	};

	class Application
	{
	  private:
		GLFWwindow* window;
		std::unique_ptr<ScoreEditor> editor;
		std::unique_ptr<ImGuiManager> imgui;
		UnsavedChangesDialog unsavedChangesDialog;

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
		void appendOpenFile(std::string filename);
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void loadResources();
		void dispose();

		GLFWwindow* getGlfwWindow() { return window; }

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}