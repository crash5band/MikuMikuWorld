#pragma once
#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "Rendering/Renderer.h"
#include "ScoreEditor.h"
#include "Stopwatch.h"
#include "ApplicationConfiguration.h"
#include "ImGuiManager.h"
#include "AutoSaveManager.h"

namespace MikuMikuWorld
{
	class Result;

	class Application
	{
	private:
		GLFWwindow* window;
		Renderer* renderer;
		ScoreEditor* editor;
		ImGuiManager imgui;
		AutoSaveManager autoSave;
		ApplicationConfiguration config;

		float lastFrame;
		float frameDelta;
		float appFrame;
		float appTime;
		float lastAppTimeUpdate;
		bool initialized;
		bool vsync;
		bool showPerformanceMetrics;
		bool aboutOpen;
		bool settingsOpen;
		bool unsavedOpen;
		bool shouldPickScore;

		std::string pendingDropScoreFile;
		static std::string version;
		static std::string appDir;

		Stopwatch stopwatch;

		Result initOpenGL();
		void update();
		void updateDialogs();
		void installCallbacks();

		std::string getVersion();

	public:
		static std::vector<std::string> pendingOpenFiles;
		static bool dragDropHandled;
		static bool exiting;
		static bool resetting;
		static bool maximized;
		static Vector2 windowPos;
		static Vector2 windowSize;

		Application(const std::string &rootPath);

		void initialize();
		void reset();
		void open();
		void run();
		void frameTime();
		void menuBar();
		bool warnUnsaved();
		void about();
		void settingsDialog();
		void processInput();
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void loadResources();
		void dispose();

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}