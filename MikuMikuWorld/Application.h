#pragma once
#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Rendering/Renderer.h"
#include "ScoreEditor.h"
#include "Stopwatch.h"
#include "ApplicationConfiguration.h"

namespace MikuMikuWorld
{
	class Application
	{
	private:
		GLFWwindow* window;
		Renderer* renderer;
		ScoreEditor* editor;
		float lastFrame;
		float frameDelta;
		float appFrame;
		float appTime;
		float lastAppTimeUpdate;
		bool initialized;
		bool firstFrame;
		bool vsync;
		bool showPerformanceMetrics;
		bool aboutOpen;
		bool settingsOpen;
		bool unsavedOpen;
		bool shouldPickScore;
		bool autoSaveEnabled;
		int autoSaveInterval; // in minutes
		int autoSaveMaxCount;
		ApplicationConfiguration config;

		std::string pendingDropScoreFile;
		std::string imguiConfigFile;
		static std::string version;
		static std::string appDir;

		ImGuiID dockspaceID;
		Stopwatch stopwatch;
		Stopwatch autoSaveTimer;

		void initLayout();
		bool initOpenGL();
		bool initImgui();
		void setImguiStyle();
		void update();
		void updateDialogs();
		void installCallbacks();

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);
		static void windowCloseCallback(GLFWwindow* window);
		static void dropCallback(GLFWwindow* window, int count, const char** paths);
		static void windowSizeCallback(GLFWwindow* window, int width, int height);
		static void windowPositionCallback(GLFWwindow* window, int x, int y);
		static void windowMaximizeCallback(GLFWwindow* window, int maximized);

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

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}