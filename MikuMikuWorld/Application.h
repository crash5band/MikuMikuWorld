#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "ScoreEditor.h"
#include "Stopwatch.h"
#include "ApplicationConfiguration.h"
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
		Vector2 position;
		Vector2 size;
	};

	class Application
	{
	private:
		GLFWwindow* window;
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

		Application(const std::string &rootPath);

		Result initialize();
		void run();
		void update();
		void frameTime();
		void appendOpenFile(std::string filename);
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void loadResources();
		void dispose();

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}