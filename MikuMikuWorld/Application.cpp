#include "Application.h"
#include "ScoreEditor.h"
#include "ResourceManager.h"
#include "IconsFontAwesome5.h"
#include "InputListener.h"
#include "FileDialog.h"
#include "UI.h"
#include "Colors.h"
#include "Utilities.h"
#include <json.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace nlohmann;

namespace MikuMikuWorld
{
	int Application::screenWidth = 1280;
	int Application::screenHeight = 720;
	std::string Application::version;
	std::string Application::appDir;

	Application::Application(const std::string& root) :
		lastAppTimeUpdate{ 0 }, appFrame{ 0 }, appTime{ 0 }, vsync{ true }, exiting{ false }
	{
		initOpenGL();
		initImgui();
		setImguiStyle();
		applyAccentColor(0);
		showDemoWindow = false;

		renderer = new Renderer();
		editor = new ScoreEditor();

		appDir = root;
		version = getVersion();

		readSettings(appDir + appConfigFilename);

		dockspaceID = 3939;
	}

	const std::string& Application::getAppDir()
	{
		return appDir;
	}

	const std::string& Application::getAppVersion()
	{
		return version;
	}

	std::string Application::getVersion()
	{
		return "3.9.3.9";
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

	void Application::warnExit()
	{
		ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_Appearing);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		if (ImGui::BeginPopupModal(ICON_FA_INFO_CIRCLE " Miku Miku World - Unsaved Changes", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("Save changes to current file?");

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));
			
			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x - spacing.x) / 3.0f, btnNormal.y);
			
			if (ImGui::Button("Save Changes", btnSz))
				editor->save();

			ImGui::SameLine();
			if (ImGui::Button("Discard Changes", btnSz))
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();
			if (ImGui::Button("Cancel", btnSz))
			{
				ImGui::CloseCurrentPopup();
				exiting = false;
			}

			ImGui::EndPopup();
		}
	}

	void Application::about()
	{
		ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_Appearing);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;

		if (ImGui::BeginPopupModal("Miku Miku World - About", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::Text("MikuMikuWorld\nCopyright (C) 2022 Crash5b\n\n");
			ImGui::Separator();

			ImGui::Text("Version %s", version.c_str());

			float xPos = padding.x;
			float yPos = ImGui::GetWindowSize().y - btnNormal.y - padding.y;
			ImGui::SetCursorPos(ImVec2(xPos, yPos));

			ImVec2 btnSz = ImVec2((ImGui::GetContentRegionAvail().x), btnNormal.y);

			if (ImGui::Button("OK", btnSz))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void Application::settings()
	{
		ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_Always);

		ImVec2 padding = ImGui::GetStyle().WindowPadding;
		ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
		ImVec2 confirmBtnPos =  ImGui::GetWindowSize() + ImVec2(-12.5f, btnNormal.y / 2.0f);

		if (ImGui::BeginPopupModal("Miku Miku World - Settings", NULL, ImGuiWindowFlags_NoResize))
		{
			ImGui::BeginChild("##settings_panel", ImGui::GetContentRegionAvail() - ImVec2(0, btnNormal.y), true);
			
			// theme
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ImGui::GetStyle().ItemSpacing.x + 3, 15));
			if (ImGui::CollapsingHeader("Accent Color", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (int i = 0; i < accentColors.size(); ++i)
				{
					bool apply = false;
					std::string id = "##" + std::to_string(i);
					ImGui::PushStyleColor(ImGuiCol_Button, accentColors[i].color);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, accentColors[i].color);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, accentColors[i].color);

					apply = ImGui::Button(id.c_str(), btnNormal);

					ImGui::PopStyleColor(3);

					if (i < accentColors.size() - 1 && ImGui::GetCursorPosX() < ImGui::GetWindowSize().x - btnNormal.x - 50.0f)
						ImGui::SameLine();

					if (apply)
						applyAccentColor(i);
				}
				ImGui::PopStyleVar();
			}

			// charting
			if (ImGui::CollapsingHeader("Timeline", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int laneWidth = editor->getLaneWidth();
				int notesHeight = editor->getNotesHeight();

				beginPropertyColumns();
				addSliderProperty("Lane Width", laneWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH, "%d");
				addSliderProperty("Notes Height", notesHeight, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT, "%d");
				endPropertyColumns();

				if (laneWidth != editor->getLaneWidth())
					editor->setLaneWidth(laneWidth);

				if (notesHeight != editor->getNotesHeight())
					editor->setNotesHeight(notesHeight);
			}

			// graphics
			if (ImGui::CollapsingHeader("Video", ImGuiTreeNodeFlags_DefaultOpen))
			{
				if (ImGui::Checkbox("Enable VSync", &vsync))
					glfwSwapInterval((int)vsync);
			}

			ImGui::EndChild();
			ImGui::SetCursorPos(confirmBtnPos);
			if (ImGui::Button("OK", ImVec2(100, btnNormal.y - 5)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	ImVec4 Application::generateDarkColor(const ImVec4& color)
	{
		return ImVec4(
			std::max(0.0f, color.x - 0.10f),
			std::max(0.0f, color.y - 0.10f),
			std::max(0.0f, color.z - 0.10f),
			1.0f);
	}

	ImVec4 Application::generateHighlightColor(const ImVec4& color)
	{
		return ImVec4(
			std::max(0.0f, color.x + 0.10f),
			std::max(0.0f, color.y + 0.10f),
			std::max(0.0f, color.z + 0.10f),
			1.0f);
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
		if (InputListener::isCtrlDown())
		{
			if (InputListener::isTapped(GLFW_KEY_N))
				editor->reset();
			else if (InputListener::isTapped(GLFW_KEY_O))
				editor->open();
			else if (InputListener::isTapped(GLFW_KEY_S))
			{
				if (InputListener::isShiftDown())
					editor->saveAs();
				else
					editor->save();
			}
			else if (InputListener::isTapped(GLFW_KEY_C))
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
			for (int k = GLFW_KEY_1; k < GLFW_KEY_1 + (int)TimelineMode::TimelineToolMax; ++k)
				if (InputListener::isTapped(k) && !ImGui::GetIO().WantCaptureKeyboard)
					editor->changeMode((TimelineMode)(k - GLFW_KEY_1));

			if (InputListener::isTapped(GLFW_KEY_DELETE))
				editor->deleteSelected();
			else if (InputListener::isTapped(GLFW_KEY_ESCAPE))
				editor->cancelPaste();
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

	void Application::menuBar()
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 5));

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl + N"))
				editor->reset();

			if (ImGui::MenuItem("Open", "Ctrl + O"))
				editor->open();

			ImGui::Separator();

			if (ImGui::MenuItem("Save", "Ctrl + S"))
				editor->save();

			if (ImGui::MenuItem("Save As", "Ctrl + Shift + S"))
				editor->saveAs();

			ImGui::Separator();

			ImGui::MenuItem("Exit", "Alt + F4");
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl + Z"))
				editor->undo();

			if (ImGui::MenuItem("Redo", "Ctrl + Y"))
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

#ifdef _DEBUG
			ImGui::MenuItem("Show Demo Window", NULL, &showDemoWindow);
#endif // _DEBUG

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
		
		if (aboutOpen)
		{
			ImGui::OpenPopup("Miku Miku World - About");
			aboutOpen = false;
		}

		if (settingsOpen)
		{
			ImGui::OpenPopup("Miku Miku World - Settings");
			settingsOpen = false;
		}

		about();
		settings();
	}

	void Application::resizeLayout(int screenWidth, int screenHeight)
	{
		float yPos = ImGui::GetFrameHeight() + 1;
		ImGui::DockBuilderSetNodePos(dockspaceID, ImVec2(0.0f, yPos));
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImVec2(screenWidth, screenHeight - yPos));
	}

	void Application::initLayout(int width, int height)
	{
		if (ImGui::DockBuilderGetNode(dockspaceID))
			return;

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImVec2(width, height - ImGui::GetFrameHeight()));

		ImGuiID mainID = dockspaceID;
		ImGuiID right, extra;
		ImGui::DockBuilderSplitNode(mainID, ImGuiDir_Right, 0.22f, &right, &mainID);
		ImGui::DockBuilderSplitNode(right, ImGuiDir_Down, 0.35f, &extra, &right);

		ImGui::DockBuilderDockWindow(timelineWindow, mainID);
		ImGui::DockBuilderDockWindow(detailsWindow, right);
		ImGui::DockBuilderDockWindow(debugWindow, extra);

		ImGui::DockBuilderFinish(dockspaceID);
	}

	void Application::run()
	{
		ResourceManager::loadTexture("res/textures/tex_notes.png");
		ResourceManager::loadTexture("res/textures/tex_hold_path.png");
		ResourceManager::loadTexture("res/textures/tex_hold_path_crtcl.png");
		ResourceManager::loadTexture("res/textures/tex_note_common_all.png");
		ResourceManager::loadTexture("res/textures/default.png");
		ResourceManager::loadTexture("res/textures/bg_sekai.png");
		ResourceManager::loadShader("res/shaders/basic2d");

		while (!glfwWindowShouldClose(window))
		{
			stopwatch.reset();

			glfwPollEvents();
			frameTime();

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			resizeLayout(screenWidth, screenHeight);
			initLayout(screenWidth, screenHeight);

			InputListener::update(window);
			processInput();
			menuBar();
			editor->update(frameDelta, renderer);

			if (glfwGetTime() - lastAppTimeUpdate >= 0.1f)
			{
				appTime = stopwatch.elapsed();
				appFrame = frameDelta;
				lastAppTimeUpdate = glfwGetTime();
			}

			if (showPerformanceMetrics)
			{
				ImGui::BeginMainMenuBar();
				ImGui::SetCursorPosX(screenWidth - 300);
				ImGui::Text("app time: %.2fms :: frame time: %.2fms (%.0fFPS)",
					appTime * 1000,
					appFrame * 1000,
					1 / appFrame
				);

				ImGui::EndMainMenuBar();
			}

			if (showDemoWindow)
				ImGui::ShowDemoWindow();

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

			glfwSwapBuffers(window);
		}

		writeSettings(appDir + appConfigFilename);

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