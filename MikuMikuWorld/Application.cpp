#include "Application.h"
#include "ResourceManager.h"
#include "StringOperations.h"
#include "InputListener.h"
#include "Colors.h"
#include "UI.h"

namespace MikuMikuWorld
{
	std::string Application::version;
	std::string Application::appDir;
	std::vector<std::string> Application::pendingOpenFiles;
	bool Application::dragDropHandled = true;
	bool Application::exiting = false;
	bool Application::resetting = false;
	bool Application::maximized = false;
	Vector2 Application::windowPos;
	Vector2 Application::windowSize;

	Application::Application(const std::string& root) :
		lastAppTimeUpdate{ 0 }, appFrame{ 0 }, appTime{ 0 }, firstFrame{ true }
	{
		appDir = root;
		version = getVersion();
		readSettings();

		if (!initOpenGL())
			exit(1);

		imguiConfigFile = std::string{ appDir + IMGUI_CONFIG_FILENAME };
		if (!initImgui())
			exit(2);

		setImguiStyle();
		applyAccentColor(config.accentColor);

		renderer = new Renderer();
		editor = new ScoreEditor();
		editor->setLaneWidth(config.timelineWidth);
		editor->setNotesHeight(config.notesHeight);
		editor->loadPresets(appDir + "library/");

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

	void Application::frameTime()
	{
		float currentFrame = glfwGetTime();
		frameDelta = currentFrame - lastFrame;
		lastFrame = currentFrame;
	}

	void Application::readSettings()
	{
		config.read(appDir + APP_CONFIG_FILENAME);

		windowPos = config.windowPos;
		windowSize = config.windowSize;
		maximized = config.maximized;
		vsync = config.vsync;
		UI::accentColors[0].color = ImVec4{
			config.userColor.r,
			config.userColor.g,
			config.userColor.b,
			config.userColor.a
		};
	}

	void Application::writeSettings()
	{
		config.maximized = maximized;
		config.version = APP_CONFIG_VERSION;
		config.vsync = vsync;
		config.windowPos = windowPos;
		config.windowSize = windowSize;
		config.timelineWidth = editor->getLaneWidth();
		config.notesHeight = editor->getNotesHeight();
		config.userColor = Color{ 
			UI::accentColors[0].color.x,
			UI::accentColors[0].color.y,
			UI::accentColors[0].color.z,
			UI::accentColors[0].color.w
		};

		config.write(appDir + APP_CONFIG_FILENAME);
	}

	void Application::applyAccentColor(int colIndex)
	{
		ImVec4* colors = ImGui::GetStyle().Colors;
		const ImVec4 color = UI::accentColors[colIndex].color;
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

		config.accentColor = colIndex;
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

			if (ImGui::MenuItem("Export"))
				editor->exportSUS();

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
		settingsDialog();

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
			else if (pendingDropScoreFile.size())
			{
				editor->loadScore(pendingDropScoreFile);
				pendingDropScoreFile = "";
				resetting = false;
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

			if (extension == "sus" || extension == "mmws")
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

		writeSettings();
		editor->savePresets(appDir + "library/");

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