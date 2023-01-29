#include "Application.h"
#include "ResourceManager.h"
#include "StringOperations.h"
#include "InputListener.h"
#include "Colors.h"
#include "Color.h"
#include "UI.h"
#include "Utilities.h"
#include "Localization.h"
#include "tinyfiledialogs.h"
#include <nfd.h>
#include <filesystem>

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
		lastAppTimeUpdate{ 0 }, appFrame{ 0 }, appTime{ 0 }, initialized{ false }
	{
		appDir = root;
		version = getVersion();
	}

	void Application::initialize()
	{
		if (initialized)
			return;

		readSettings();

		Result openglResult = initOpenGL();
		if (!openglResult.isOk())
		{
			tinyfd_messageBox(APP_NAME, openglResult.getMessage().c_str(), "ok", "error", 1);
			exit(1);
		}

		imgui.initialize(window);
		applyAccentColor(config.accentColor);

		renderer = new Renderer();
		editor = new ScoreEditor();

		// apply config settings
		editor->setDivision(config.division);
		editor->setScrollMode(config.scrollMode);
		editor->canvas.setLaneWidth(config.timelineWidth);
		editor->canvas.setNotesHeight(config.notesHeight);
		editor->canvas.setZoom(config.zoom);
		editor->canvas.setUseSmoothScrolling(config.useSmoothScrolling);
		editor->canvas.setSmoothScrollingTime(config.smoothScrollingTime);
		editor->canvas.setLaneTransparency(config.timelineTansparency);
		editor->canvas.setBackgroundBrightness(config.backgroundBrightness);

		editor->audio.setMasterVolume(config.masterVolume);
		editor->audio.setBGMVolume(config.bgmVolume);
		editor->audio.setSEVolume(config.seVolume);

		editor->presetManager.loadPresets(appDir + "library/");

		loadResources();
		int bgTex = ResourceManager::getTexture("default");
		if (bgTex != -1)
			editor->canvas.changeBackground(ResourceManager::textures[bgTex]);

		NFD_Init();
		initialized = true;
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

	void Application::dispose()
	{
		if (initialized)
		{
			imgui.shutdown();
			glfwDestroyWindow(window);
			glfwTerminate();
		}
		initialized = false;
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

		autoSaveEnabled = config.autoSaveEnabled;
		autoSaveInterval = config.autoSaveInterval;
		autoSaveMaxCount = config.autoSaveMaxCount;
	}

	void Application::writeSettings()
	{
		config.maximized = maximized;
		config.vsync = vsync;
		config.windowPos = windowPos;
		config.windowSize = windowSize;
		config.userColor = Color{
			UI::accentColors[0].color.x,
			UI::accentColors[0].color.y,
			UI::accentColors[0].color.z,
			UI::accentColors[0].color.w
		};

		config.timelineWidth = editor->canvas.getLaneWidth();
		config.notesHeight = editor->canvas.getNotesHeight();
		config.zoom = editor->canvas.getZoom();
		config.division = editor->getDivision();
		config.scrollMode = scrollModes[(int)editor->getScrollMode()];
		config.timelineTansparency = editor->canvas.getLaneTransparency();
		config.backgroundBrightness = editor->canvas.getBackgroundBrightness();
		config.useSmoothScrolling = editor->canvas.isUseSmoothScrolling();
		config.smoothScrollingTime = editor->canvas.getSmoothScrollingTime();

		config.autoSaveEnabled = autoSaveEnabled;
		config.autoSaveInterval = autoSaveInterval;
		config.autoSaveMaxCount = autoSaveMaxCount;

		config.masterVolume = editor->audio.getMasterVolume();
		config.bgmVolume = editor->audio.getBGMVolume();
		config.seVolume = editor->audio.getSEVolume();

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
			else if (InputListener::isTapped(GLFW_KEY_H))
				if (InputListener::isShiftDown())
					editor->shrinkSelected(1);
				else
					editor->shrinkSelected(0);
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

			for (int k = GLFW_KEY_KP_1; k < GLFW_KEY_KP_1 + (int)TimelineMode::TimelineToolMax; ++k)
				if (InputListener::isTapped(k))
					editor->changeMode((TimelineMode)(k - GLFW_KEY_KP_1));

			if (InputListener::isTapped(GLFW_KEY_DELETE))
				editor->deleteSelected();
			else if (InputListener::isTapped(GLFW_KEY_DOWN) && !editor->isPlaying())
				editor->previousTick();
			else if (InputListener::isTapped(GLFW_KEY_SPACE))
			{
				if (InputListener::isShiftDown())
					editor->stopAtLastSelectedTick();
				else
					editor->togglePlaying();
			}
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

	void Application::autoSave()
	{
		std::string autoSaveDir = appDir + "auto_save/";
		std::wstring wAutoSaveDir = mbToWideStr(autoSaveDir);

		// create auto save directory if none exists
		if (!std::filesystem::exists(wAutoSaveDir))
			std::filesystem::create_directory(wAutoSaveDir);

		editor->save(autoSaveDir + "mmw_auto_save_" + Utilities::getCurrentDateTime() + MMWS_EXTENSION);

		// get mmws files
		int mmwsCount = 0;
		for (const auto& file : std::filesystem::directory_iterator(wAutoSaveDir))
		{
			std::string extension = file.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			mmwsCount += extension == MMWS_EXTENSION;
		}

		// delete older files
		if (mmwsCount > autoSaveMaxCount)
			deleteOldAutoSave(autoSaveDir, mmwsCount - autoSaveMaxCount);
	}

	void Application::deleteOldAutoSave(const std::string& path, int count)
	{
		std::wstring wAutoSaveDir = mbToWideStr(path);
		if (!std::filesystem::exists(wAutoSaveDir))
			return;

		// get mmws files
		using entry = std::filesystem::directory_entry;
		std::vector<entry> deleteFiles;
		for (const auto& file : std::filesystem::directory_iterator(wAutoSaveDir))
		{
			std::string extension = file.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			if (extension == MMWS_EXTENSION)
				deleteFiles.push_back(file);
		}

		// sort files by modification date
		std::sort(deleteFiles.begin(), deleteFiles.end(), [](const entry& f1, const entry& f2){
			return f1.last_write_time() < f2.last_write_time();
		});

		while (count)
		{
			std::filesystem::remove(deleteFiles.begin()->path());
			deleteFiles.erase(deleteFiles.begin());
			--count;
		}
	}

	void Application::menuBar()
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 3));
		
		if (ImGui::BeginMenu(getString("file")))
		{
			if (ImGui::MenuItem(getString("new"), "Ctrl + N"))
				reset();

			if (ImGui::MenuItem(getString("open"), "Ctrl + O"))
				open();

			ImGui::Separator();

			if (ImGui::MenuItem(getString("save"), "Ctrl + S"))
				editor->save();

			if (ImGui::MenuItem(getString("save_as"), "Ctrl + Shift + S"))
				editor->saveAs();

			if (ImGui::MenuItem(getString("export")))
				editor->exportSUS();

			ImGui::Separator();

			if (ImGui::MenuItem(getString("exit"), "Alt + F4"))
				exiting = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("edit")))
		{
			if (ImGui::MenuItem(getString("undo"), "Ctrl + Z", false, editor->history.hasUndo()))
				editor->undo();

			if (ImGui::MenuItem(getString("redo"), "Ctrl + Y", false, editor->history.hasRedo()))
				editor->redo();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("select_all"), "Ctrl + A"))
				editor->selectAll();

			if (ImGui::MenuItem(getString("settings")))
				settingsOpen = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("window")))
		{
			if (ImGui::MenuItem(getString("vsync"), NULL, &vsync))
				glfwSwapInterval((int)vsync);

			ImGui::MenuItem(getString("show_performance"), NULL, &showPerformanceMetrics);

			ImGui::EndMenu();
		}

#ifdef _DEBUG
		if (ImGui::BeginMenu(getString("debug")))
		{
			if (ImGui::MenuItem(getString("create_auto_save")))
				autoSave();

			ImGui::EndMenu();
		}
#endif // _DEBUG

		if (ImGui::BeginMenu(getString("about")))
		{
			if (ImGui::MenuItem(getString("about")))
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
			ImGui::OpenPopup(MODAL_TITLE("about"));
			aboutOpen = false;
		}

		if (settingsOpen)
		{
			ImGui::OpenPopup(MODAL_TITLE("settings"));
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
					ImGui::OpenPopup(MODAL_TITLE("unsaved_changes"));
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
					ImGui::OpenPopup(MODAL_TITLE("unsaved_changes"));
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

	void Application::handlePendingOpenFiles()
	{
		std::string scoreFile = "";
		std::string musicFile = "";

		for (auto it = pendingOpenFiles.rbegin(); it != pendingOpenFiles.rend(); ++it)
		{
			std::string extension = File::getFileExtension(*it);
			std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

			if (extension == SUS_EXTENSION || extension == MMWS_EXTENSION)
				scoreFile = *it;
			else if (extension == ".mp3" || extension == ".wav" || extension == ".flac" || extension == ".ogg")
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
		imgui.initializeLayout();
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

		if (glfwGetTime() - lastAppTimeUpdate >= 0.05f)
		{
			appTime = stopwatch.elapsed();
			appFrame = frameDelta;
			lastAppTimeUpdate = glfwGetTime();
		}

		if (autoSaveEnabled && autoSaveTimer.elapsedMinutes() >= autoSaveInterval)
		{
			autoSave();
			autoSaveTimer.reset();
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

	void Application::loadResources()
	{
		ResourceManager::loadShader(appDir + "res/shaders/basic2d");
		ResourceManager::loadTexture(appDir + "res/textures/tex_notes.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_hold_path.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_hold_path_crtcl.png");
		ResourceManager::loadTexture(appDir + "res/textures/default.png");

		// load more languages here
		Localization::loadDefault();
		Localization::load("ja", appDir + "res/i18n/ja.csv");

		std::string locale = Utilities::getSystemLocale();
		if (!Localization::setLanguage(locale))
		{
			// fallback to default language if language resource file is not found.
			Localization::setLanguage("en");
		}
	}

	void Application::run()
	{
		autoSaveTimer.reset();

		while (!glfwWindowShouldClose(window))
		{
			stopwatch.reset();

			glfwPollEvents();
			frameTime();

			imgui.begin();
			update();

			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			imgui.draw(window);
			glfwSwapBuffers(window);
		}

		writeSettings();
		editor->presetManager.savePresets(appDir + "library/");

		NFD_Quit();
	}
}
