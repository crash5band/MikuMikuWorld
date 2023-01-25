#pragma once
#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "Rendering/Renderer.h"
#include "ScoreEditor.h"
#include "Stopwatch.h"
#include "ApplicationConfiguration.h"
#include "ImGuiManager.h"

namespace MikuMikuWorld
{
	class Application
	{
	private:
		GLFWwindow* window;
		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<ScoreEditor> editor;
		ImGuiManager imgui;
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
		bool autoSaveEnabled;
		int autoSaveInterval; // in minutes
		int autoSaveMaxCount;

		std::string pendingDropScoreFile;
		static std::string version;
		static std::string appDir;

		Stopwatch stopwatch;
		Stopwatch autoSaveTimer;

		bool initOpenGL();
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
		void applyAccentColor(int col);
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void autoSave();
		void deleteOldAutoSave(const std::string& path, int count);
		void loadResources();
		void dispose();

		bool trySetLanguage(const char* code);

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}