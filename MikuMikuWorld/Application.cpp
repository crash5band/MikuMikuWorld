#include "Application.h"
#include "ResourceManager.h"
#include "InputListener.h"
#include "UI.h"
#include "Colors.h"
#include <json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace nlohmann;

namespace MikuMikuWorld
{
	std::string Application::version;
	std::string Application::appDir;
	std::vector<std::string> Application::pendingOpenFiles;
	bool Application::dragDropHandled = true;
	bool Application::exiting = false;
	bool Application::resetting = false;

	Application::Application(const std::string& root) :
		lastAppTimeUpdate{ 0 }, appFrame{ 0 }, appTime{ 0 }, vsync{ true }, firstFrame{ true }
	{
		appDir = root;
		version = getVersion();

		initOpenGL();
		initImgui();
		setImguiStyle();
		applyAccentColor(0);

		imguiConfigFile = std::string{ appDir + IMGUI_CONFIG_FILENAME };
		ImGui::GetIO().IniFilename = imguiConfigFile.c_str();

		renderer = new Renderer();
		editor = new ScoreEditor();
		dockspaceID = 3939;

		readSettings(appDir + APP_CONFIG_FILENAME);
	}

	const std::string& Application::getAppDir()
	{
		return appDir;
	}

	const std::string& Application::getAppVersion()
	{
		return version;
	}

	void Application::readSettings(const std::string& filename)
	{
		if (!std::filesystem::exists(filename))
			return;
		
		std::ifstream configFile(filename);
		json config;
		configFile >> config;
		configFile.close();

		int posX = config["window"]["position"]["x"];
		int posY = config["window"]["position"]["y"];
		int sizeX = config["window"]["size"]["x"];
		int sizeY = config["window"]["size"]["y"];

		bool maximized = config["window"]["maximized"];
		if (maximized)
			glfwMaximizeWindow(window);

		vsync = config["vsync"];
		glfwSwapInterval(vsync);

		editor->setLaneWidth(config["timeline"]["lane_width"]);
		editor->setNotesHeight(config["timeline"]["notes_height"]);

		applyAccentColor(config["accent_color"]);
	}

	void Application::writeSettings(const std::string& filename)
	{
		int posX = 0, posY = 0, sizeX = 0, sizeY = 0;
		glfwGetWindowPos(window, &posX, &posY);
		glfwGetWindowSize(window, &sizeX, &sizeY);
		bool maximized = glfwGetWindowAttrib(window, GLFW_MAXIMIZED);

		json config = {
			{"window", {
				{"position", {{"x", posX}, {"y", posY}}},
				{"size", {{"x", sizeX}, {"y", sizeY}}},
				{"maximized", maximized}}
			},

			{"timeline", {
				{"lane_width", (int)editor->getLaneWidth()},
				{"notes_height", (int)editor->getNotesHeight()}}
			},

			{"accent_color", accentColor},
			{"vsync", vsync}
		};

		std::ofstream configFile(filename);
		configFile << std::setw(4) << config;
		configFile.close();
	}

	void Application::frameTime()
	{
		float currentFrame = glfwGetTime();
		frameDelta = currentFrame - lastFrame;
		lastFrame = currentFrame;
	}

	void Application::applyAccentColor(int index)
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		const ImVec4& color = accentColors[index].color;
		const ImVec4 darkColor = generateDarkColor(color);
		const ImVec4 lightColor = generateHighlightColor(color);

		colors[ImGuiCol_SliderGrab] = color;
		colors[ImGuiCol_SliderGrabActive] = darkColor;
		colors[ImGuiCol_ButtonActive] = darkColor;
		colors[ImGuiCol_Header] = color;
		colors[ImGuiCol_HeaderHovered] = lightColor;
		colors[ImGuiCol_HeaderActive] = darkColor;
		colors[ImGuiCol_SeparatorHovered] = lightColor;
		colors[ImGuiCol_TabHovered] = lightColor;
		colors[ImGuiCol_TabActive] = color;
		colors[ImGuiCol_CheckMark] = color;

		accentColor = index;
	}

	void Application::processInput()
	{
		if (ImGui::GetIO().WantCaptureKeyboard)
			return;

		if (InputListener::isCtrlDown())
		{
			if (InputListener::isTapped(GLFW_KEY_N))
				reset();
			else if (InputListener::isTapped(GLFW_KEY_O))
				open();
			else if (InputListener::isTapped(GLFW_KEY_S))
			{
				if (InputListener::isShiftDown())
					editor->saveAs();
				else
					editor->save();
			}

			if (InputListener::isTapped(GLFW_KEY_C))
				editor->copy();
			else if (InputListener::isTapped(GLFW_KEY_V))
			{
				if (InputListener::isShiftDown())
					editor->flipPaste();
				else
					editor->paste();
			}
			else if (InputListener::isTapped(GLFW_KEY_Z))
				editor->undo();
			else if (InputListener::isTapped(GLFW_KEY_Y))
				editor->redo();
			else if (InputListener::isTapped(GLFW_KEY_F))
				editor->flipSelected();
			else if (InputListener::isTapped(GLFW_KEY_A) && !editor->isPlaying())
				editor->selectAll();
		}
		else
		{
			if (InputListener::isTapped(GLFW_KEY_ESCAPE))
				editor->cancelPaste();

			for (int k = GLFW_KEY_1; k < GLFW_KEY_1 + (int)TimelineMode::TimelineToolMax; ++k)
				if (InputListener::isTapped(k))
					editor->changeMode((TimelineMode)(k - GLFW_KEY_1));

			if (InputListener::isTapped(GLFW_KEY_DELETE))
				editor->deleteSelected();
			else if (InputListener::isTapped(GLFW_KEY_DOWN) && !editor->isPlaying())
				editor->previousTick();
			else if (InputListener::isTapped(GLFW_KEY_SPACE))
				editor->togglePlaying();
			else if (InputListener::isTapped(GLFW_KEY_BACKSPACE))
				editor->stop();
			else if (InputListener::isTapped(GLFW_KEY_UP) && !editor->isPlaying())
				editor->nextTick();
			else if (InputListener::isTapped(GLFW_KEY_A) && editor->isPlaying())
				editor->insertNotePlaying();
		}
	}

	void Application::reset()
	{
		if (!editor->isUptoDate())
			resetting = true;
		else
			editor->reset();
	}

	void Application::open()
	{
		resetting = true;
		shouldPickScore = true;
	}

	void Application::menuBar()
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 3));
		
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl + N"))
				reset();

			if (ImGui::MenuItem("Open", "Ctrl + O"))
				open();

			ImGui::Separator();

			if (ImGui::MenuItem("Save", "Ctrl + S"))
				editor->save();

			if (ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
				editor->saveAs();

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt + F4"))
				exiting = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			std::string undoText{ "Undo " + editor->getNextUndo() };
			std::string redoText{ "Redo " + editor->getNextRedo() };
			if (ImGui::MenuItem(undoText.c_str(), "Ctrl + Z", false, editor->hasUndo()))
				editor->undo();

			if (ImGui::MenuItem(redoText.c_str(), "Ctrl + Y", false, editor->hadRedo()))
				editor->redo();

			ImGui::Separator();
			if (ImGui::MenuItem("Select All", "Ctrl + A"))
				editor->selectAll();

			if (ImGui::MenuItem("Settings"))
				settingsOpen = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("VSync", NULL, &vsync))
				glfwSwapInterval((int)vsync);

			ImGui::MenuItem("Show Performance Metrics", NULL, &showPerformanceMetrics);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("About"))
		{
			if (ImGui::MenuItem("About"))
				aboutOpen = true;

			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}

	void Application::updateDialogs()
	{
		if (aboutOpen)
		{
			ImGui::OpenPopup(aboutModalTitle);
			aboutOpen = false;
		}

		if (settingsOpen)
		{
			ImGui::OpenPopup(settingsModalTitle);
			settingsOpen = false;
		}

		about();
		settings();

		if (resetting)
		{
			if (!editor->isUptoDate())
			{
				if (!unsavedOpen)
				{
					ImGui::OpenPopup(unsavedModalTitle);
					unsavedOpen = true;
				}

				if (warnUnsaved())
				{
					if (pendingDropScoreFile.size())
					{
						editor->loadScore(pendingDropScoreFile);
						pendingDropScoreFile = "";
					}
					else if (shouldPickScore)
					{
						editor->open();
						shouldPickScore = false;
					}
					else
					{
						editor->reset();
					}

					resetting = false;
				}
			}
			else if (shouldPickScore)
			{
				editor->open();
				resetting = false;
				shouldPickScore = false;
			}
		}

		if (exiting)
		{
			if (!editor->isUptoDate())
			{
				if (!unsavedOpen)
				{
					ImGui::OpenPopup(unsavedModalTitle);
					unsavedOpen = true;
				}

				if (warnUnsaved())
					glfwSetWindowShouldClose(window, 1);
			}
			else
			{
				glfwSetWindowShouldClose(window, 1);
			}
		}
	}

	void Application::initLayout()
	{
		dockspaceID = ImGui::DockSpaceOverViewport();

		if (ImGui::DockBuilderGetNode(dockspaceID))
			return;

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);
		ImGui::DockBuilderFinish(dockspaceID);
	}

	void Application::handlePendingOpenFiles()
	{
		std::string scoreFile = "";
		std::string musicFile = "";

		for (auto it = pendingOpenFiles.rbegin(); it != pendingOpenFiles.rend(); ++it)
		{
			std::string extension = File::getFileExtension(*it);
			std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

			if (extension == "sus")
				scoreFile = *it;
			else if (extension == "mp3" || extension == "wav" || extension == "flac" || extension == "ogg")
				musicFile = *it;

			if (scoreFile.size() && musicFile.size())
				break;
		}

		if (scoreFile.size())
		{
			resetting = true;
			pendingDropScoreFile = scoreFile;
		}

		if (musicFile.size())
			editor->loadMusic(musicFile);

		pendingOpenFiles.clear();
	}

	void Application::update()
	{
		initLayout();

		if (!dragDropHandled)
		{
			handlePendingOpenFiles();
			dragDropHandled = true;
		}

		InputListener::update(window);
		processInput();
		menuBar();
		editor->update(frameDelta, renderer);
		updateDialogs();

		if (firstFrame)
		{
			ImGui::SetWindowFocus(timelineWindow);
			firstFrame = false;
		}

		if (glfwGetTime() - lastAppTimeUpdate >= 0.05f)
		{
			appTime = stopwatch.elapsed();
			appFrame = frameDelta;
			lastAppTimeUpdate = glfwGetTime();
		}

		if (showPerformanceMetrics)
		{
			ImGui::BeginMainMenuBar();
			ImGui::SetCursorPosX(ImGui::GetMainViewport()->WorkSize.x - 300);
			ImGui::Text("app time: %.2fms :: frame time: %.2fms (%.0fFPS)",
				appTime * 1000,
				appFrame * 1000,
				1 / appFrame
			);

			ImGui::EndMainMenuBar();
		}
	}

	void Application::run()
	{
		ResourceManager::loadTexture(appDir + "res/textures/tex_notes.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_hold_path.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_hold_path_crtcl.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_note_common_all.png");
		ResourceManager::loadTexture(appDir + "res/textures/default.png");
		ResourceManager::loadShader(appDir + "res/shaders/basic2d");

		while (!glfwWindowShouldClose(window))
		{
			stopwatch.reset();

			glfwPollEvents();
			frameTime();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			update();

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			
			if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
				glfwMakeContextCurrent(window);
			}

			glfwSwapBuffers(window);
		}

		writeSettings(appDir + APP_CONFIG_FILENAME);

		// cleanup
		delete editor;
		delete renderer;

		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		glfwDestroyWindow(window);
		glfwTerminate();
	}
}