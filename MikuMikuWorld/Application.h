#pragma once
#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "ImGui/imgui_impl_glfw.h"
#include "ImGui/imgui_impl_opengl3.h"
#include "Rendering/Renderer.h"
#include "ScoreEditor.h"
#include "Stopwatch.h"

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
		bool firstFrame;
		bool vsync;
		bool showPerformanceMetrics;
		bool aboutOpen;
		bool settingsOpen;
		bool unsavedOpen;
		bool shouldPickScore;
		int accentColor;

		std::string pendingDropScoreFile;
		std::string imguiConfigFile;
		static std::string version;
		static std::string appDir;

		ImGuiID dockspaceID;
		Stopwatch stopwatch;

		void initLayout();
		bool initOpenGL();
		bool initImgui();
		void setImguiStyle();
		void update();
		void updateDialogs();

		std::string getVersion();

	public:
		static std::vector<std::string> pendingOpenFiles;
		static bool dragDropHandled;
		static bool exiting;
		static bool resetting;

		Application(const std::string &rootPath);

		void reset();
		void open();
		void run();
		void frameTime();
		void menuBar();
		bool warnUnsaved();
		void about();
		void settings();
		void processInput();
		void applyAccentColor(int col);
		void handlePendingOpenFiles();

		void readSettings(const std::string& filename);
		void writeSettings(const std::string& filename);

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}