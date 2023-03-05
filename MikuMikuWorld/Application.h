#pragma once
#include "../Depends/glad/include/glad/glad.h"
#include "../Depends/GLFW/include/GLFW/glfw3.h"
#include "Rendering/Renderer.h"
#include "ScoreEditor.h"
#include "Stopwatch.h"
#include "ApplicationConfiguration.h"
#include "ImGuiManager.h"
#include "AutoSaveManager.h"
#include "Commands/CommandManager.h"
#include "Menubar.h"
#include "WindowState.h"

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
		CommandManager commandManager;
		Menubar menubar;

		float lastFrame;
		float frameDelta;
		bool initialized;
		bool resetting;
		
		bool shouldPickScore;
		bool dragDropHandled;

		std::vector<std::string> pendingOpenFiles;
		std::string pendingDropScoreFile;

		static std::string version;
		static std::string appDir;

		Result initOpenGL();
		void update();
		void updateDialogs();
		void installCallbacks();

		std::string getVersion();

	public:
		WindowState windowState;

		Application(const std::string &rootPath);

		Result initialize();
		void reset();
		void open();
		void run();
		void frameTime();
		bool warnUnsaved();
		void about();
		void settingsDialog();
		void appendOpenFile(std::string filename);
		void handlePendingOpenFiles();
		void readSettings();
		void writeSettings();
		void loadResources();
		void setupCommands();
		void updateToolbar();
		void dispose();

		static const std::string& getAppDir();
		static const std::string& getAppVersion();
	};
}