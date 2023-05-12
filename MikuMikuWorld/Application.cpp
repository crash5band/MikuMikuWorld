#include "Application.h"
#include "ResourceManager.h"
#include "IO.h"
#include "Colors.h"
#include "UI.h"
#include "Utilities.h"
#include "Localization.h"
#include "Constants.h"
#include <filesystem>
#include <Windows.h>

namespace MikuMikuWorld
{
	std::string Application::version;
	std::string Application::appDir;
	WindowState Application::windowState;

	Application::Application(const std::string& root) : initialized{ false }
	{
		appDir = root;
		version = getVersion();
	}

	Result Application::initialize()
	{
		if (initialized)
			return Result(ResultStatus::Success, "App is already initialized");

		config.read(appDir + APP_CONFIG_FILENAME);
		readSettings();

		Result result = initOpenGL();
		if (!result.isOk())
			return result;

		imgui = std::make_unique<ImGuiManager>();
		result = imgui->initialize(window);
		if (!result.isOk())
			return result;

		imgui->setBaseTheme(config.baseTheme);
		imgui->applyAccentColor(config.accentColor);

		loadResources();

		editor = std::make_unique<ScoreEditor>();
		editor->loadPresets(appDir + "library");

		initialized = true;
		return Result::Ok();;
	}

	const std::string& Application::getAppDir()
	{
		return appDir;
	}

	std::string Application::getVersion()
	{
		wchar_t filename[1024];
		lstrcpyW(filename, IO::mbToWideStr(std::string(appDir + "MikuMikuWorld.exe")).c_str());

		DWORD  verHandle = 0;
		UINT   size = 0;
		LPBYTE lpBuffer = NULL;
		DWORD  verSize = GetFileVersionInfoSizeW(filename, &verHandle);

		int major = 0, minor = 0, build = 0, rev = 0;
		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];

			if (GetFileVersionInfoW(filename, verHandle, verSize, verData))
			{
				if (VerQueryValue(verData, "\\", (VOID FAR * FAR*) & lpBuffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
						if (verInfo->dwSignature == 0xfeef04bd)
						{
							major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
							minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
							rev = (verInfo->dwFileVersionLS >> 16) & 0xffff;
						}
					}
				}
			}
			delete[] verData;
		}

		return IO::formatString("%d.%d.%d", major, minor, rev);
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
			editor->uninitialize();
			imgui->shutdown();
			glfwDestroyWindow(window);
			glfwTerminate();
		}
		initialized = false;
	}

	void Application::readSettings()
	{
		windowState.position = config.windowPos;
		windowState.size = config.windowSize;
		windowState.maximized = config.maximized;
		windowState.vsync = config.vsync;
		UI::accentColors[0] = config.userColor.toImVec4();
	}

	void Application::writeSettings()
	{
		config.maximized = windowState.maximized;
		config.vsync = windowState.vsync;
		config.windowPos = windowState.position;
		config.windowSize = windowState.size;
		config.userColor = Color::fromImVec4(UI::accentColors[0]);

		config.write(appDir + APP_CONFIG_FILENAME);
	}

	void Application::appendOpenFile(std::string filename)
	{
		pendingOpenFiles.push_back(filename);
		windowState.dragDropHandled = false;
	}

	void Application::handlePendingOpenFiles()
	{
		std::string scoreFile = "";
		std::string musicFile = "";

		for (auto it = pendingOpenFiles.rbegin(); it != pendingOpenFiles.rend(); ++it)
		{
			std::string extension = IO::File::getFileExtension(*it);
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
			windowState.resetting = true;
			pendingDropScoreFile = scoreFile;
		}

		if (musicFile.size())
			editor->loadMusic(musicFile);

		pendingOpenFiles.clear();
		windowState.dragDropHandled = true;
	}

	void Application::update()
	{
		frameTime();
		imgui->begin();

		if (!windowState.dragDropHandled)
			handlePendingOpenFiles();

		imgui->initializeLayout();

		if (config.accentColor != imgui->getAccentColor())
			imgui->applyAccentColor(config.accentColor);

		if (config.userColor != Color::fromImVec4(UI::accentColors[0]) && config.accentColor == 0)
			imgui->applyAccentColor(config.accentColor);

		if (config.baseTheme != imgui->getBaseTheme())
			imgui->setBaseTheme(config.baseTheme);

		if ((windowState.closing || windowState.resetting) && !editor->isUpToDate() && !unsavedChangesDialog.open)
		{
			unsavedChangesDialog.open = true;
			ImGui::OpenPopup(MODAL_TITLE("unsaved_changes"));
		}

		auto unsavedChangesResult = unsavedChangesDialog.update();

		if (windowState.closing)
		{
			if (!editor->isUpToDate())
			{
				switch (unsavedChangesResult)
				{
				case DialogResult::Yes: editor->trySave(editor->getWorkingFilename()); glfwSetWindowShouldClose(window, 1); break;
				case DialogResult::No: glfwSetWindowShouldClose(window, 1); break;
				case DialogResult::Cancel: windowState.closing = false; break;
				default: break;
				}
			}
			else
			{
				glfwSetWindowShouldClose(window, 1);
			}
		}

		if (windowState.resetting)
		{
			if (!editor->isUpToDate())
			{
				switch (unsavedChangesResult)
				{
				case DialogResult::Yes: editor->trySave(editor->getWorkingFilename()); break;
				case DialogResult::Cancel: windowState.resetting = shouldPickScore = false; pendingDropScoreFile.clear(); break;
				default: break;
				}
			}

			// already saved or clicked save changes or discard changes
			if (editor->isUpToDate() || (unsavedChangesResult != DialogResult::Cancel && unsavedChangesResult != DialogResult::None))
			{
				editor->create();
				windowState.resetting = false;

				if (pendingDropScoreFile.size())
				{
					editor->loadScore(pendingDropScoreFile);
					pendingDropScoreFile.clear();
				}

				if (windowState.shouldPickScore)
				{
					editor->open();
					windowState.shouldPickScore = false;
				}
			}
		}

		editor->update();

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		imgui->draw(window);
		glfwSwapBuffers(window);
	}

	void Application::loadResources()
	{
		ResourceManager::loadShader(appDir + "res/shaders/basic2d");
		ResourceManager::loadTexture(appDir + "res/textures/tex_notes.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_hold_path.png");
		ResourceManager::loadTexture(appDir + "res/textures/tex_hold_path_crtcl.png");
		ResourceManager::loadTexture(appDir + "res/textures/default.png");

		ResourceManager::loadTexture(appDir + "res/textures/timeline_select.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_tap.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_hold.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_hold_step.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_flick.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_critical.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_bpm.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_time_signature.png");
		ResourceManager::loadTexture(appDir + "res/textures/timeline_hi_speed.png");

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
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			update();
		}

		editor->savePresets(appDir + "library");
		writeSettings();
	}
}
