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
		bool exiting;
		bool upToDate;
		bool showDemoWindow;
		bool showPerformanceMetrics;
		bool aboutOpen;
		bool settingsOpen;
		int accentColor;

		static std::string version;
		static std::string appDir;
		std::string imguiConfigFile;
		const std::string appConfigFilename = "app_config.json";

		ImGuiID dockspaceID;
		Stopwatch stopwatch;

		void resizeLayout(int width, int height);
		void initLayout(int width, int height);
		bool initOpenGL();
		bool initImgui();
		void setImguiStyle();
		ImVec4 generateDarkColor(const ImVec4& color);
		ImVec4 generateHighlightColor(const ImVec4& color);

		std::string getVersion();

	public:
		static int screenWidth;
		static int screenHeight;
		static std::vector<std::string> pendingOpenFiles;
		static bool dragDropHandled;

		Application(const std::string &rootPath);

		void run();
		void frameTime();
		void menuBar();
		void warnExit();
		void about();
		void settings();
		void processInput();
		void applyAccentColor(int index);
		void handlePendingOpenFiles();

		void readSettings(const std::string& filename);
		void writeSettings(const std::string& filename);

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}