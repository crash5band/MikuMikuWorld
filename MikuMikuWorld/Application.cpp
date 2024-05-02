#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "IO.h"
#include "Localization.h"
#include "ResourceManager.h"
#include "Utilities.h"
#include <filesystem>
#include <json.hpp>
#include <stdexcept>
#include <thread>

namespace MikuMikuWorld
{
	std::string Application::version;
	std::string Application::appDir;
	std::string Application::pendingLoadScoreFile;
	WindowState Application::windowState;

	NoteTextures noteTextures{ -1, -1, -1, -1, -1 };

	Application::Application() : initialized{ false }
	{
		appDir = "";
		version = "";
		language = "";
	}

	Result Application::initialize(const std::string& root)
	{
		if (initialized)
			return Result(ResultStatus::Success, "App is already initialized");

		appDir = root;
		version = getVersion();
		language = "";

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
		return Result::Ok();
	}

	const std::string& Application::getAppDir() { return appDir; }

	std::string Application::getVersion()
	{
		wchar_t filename[1024];
		lstrcpyW(filename, IO::mbToWideStr(std::string(appDir + "MikuMikuWorld.exe")).c_str());

		DWORD verHandle = 0;
		UINT size = 0;
		LPBYTE lpBuffer = NULL;
		DWORD verSize = GetFileVersionInfoSizeW(filename, &verHandle);

		int major = 0, minor = 0, build = 0, rev = 0;
		if (verSize != NULL)
		{
			LPSTR verData = new char[verSize];

			if (GetFileVersionInfoW(filename, verHandle, verSize, verData))
			{
				if (VerQueryValue(verData, "\\", (VOID FAR * FAR*)&lpBuffer, &size))
				{
					if (size)
					{
						VS_FIXEDFILEINFO* verInfo = (VS_FIXEDFILEINFO*)lpBuffer;
						if (verInfo->dwSignature == 0xfeef04bd)
						{
							major = (verInfo->dwFileVersionMS >> 16) & 0xffff;
							minor = (verInfo->dwFileVersionMS >> 0) & 0xffff;
							rev = (verInfo->dwFileVersionLS >> 16) & 0xffff;
							build = (verInfo->dwFileVersionLS >> 0) & 0xffff;
						}
					}
				}
			}
			delete[] verData;
		}

		return IO::formatString("%d.%d.%d.%d", major, minor, rev, build);
	}

	const std::string& Application::getAppVersion() { return version; }

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

		editor->writeSettings();
		config.write(appDir + APP_CONFIG_FILENAME);
	}

	void Application::appendOpenFile(const std::string& filename)
	{
		pendingOpenFiles.push_back(filename);
		windowState.dragDropHandled = false;
	}

	void Application::handlePendingOpenFiles()
	{
		std::string scoreFile{};
		std::string musicFile{};

		for (auto it = pendingOpenFiles.rbegin(); it != pendingOpenFiles.rend(); ++it)
		{
			std::string extension = IO::File::getFileExtension(*it);
			std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

			if (extension == SUS_EXTENSION || extension == USC_EXTENSION ||
			    extension == MMWS_EXTENSION || extension == CC_MMWS_EXTENSION)
				scoreFile = *it;
			else if (Audio::isSupportedFileFormat(extension))
				musicFile = *it;

			if (!scoreFile.empty() && !musicFile.empty())
				break;
		}

		if (!scoreFile.empty())
		{
			windowState.resetting = true;
			pendingLoadScoreFile = scoreFile;
		}

		if (!musicFile.empty())
			editor->loadMusic(musicFile);

		pendingOpenFiles.clear();
		windowState.dragDropHandled = true;
	}

	void Application::update()
	{
		if (config.language != language)
		{
			std::string locale =
			    config.language == "auto" ? Utilities::getSystemLocale() : config.language;

			// Try to set the selected language and fallback to default (en) on failure
			if (!Localization::setLanguage(locale))
				Localization::setLanguage("en");

			language = config.language;
		}

		float dpiX = 1.0f, dpiY = 1.0f;
		GLFWmonitor* mainMonitor = glfwGetPrimaryMonitor();
		if (mainMonitor)
		{
			glfwGetMonitorContentScale(mainMonitor, &dpiX, &dpiY);
		}

		float dpiScale = (dpiX + dpiY) * 0.5f;
		if (dpiScale != windowState.lastDpiScale)
		{
			imgui->buildFonts(dpiScale);
			windowState.lastDpiScale = dpiScale;
		}

		imgui->begin();

		// Inform ImGui of dpi changes
		ImGui::GetMainViewport()->DpiScale = dpiX;
		UI::updateBtnSizesDpiScaling(dpiScale);

		if (!windowState.dragDropHandled)
			handlePendingOpenFiles();

		imgui->initializeLayout();

		if (config.accentColor != imgui->getAccentColor())
			imgui->applyAccentColor(config.accentColor);

		if (config.userColor != Color::fromImVec4(UI::accentColors[0]) && config.accentColor == 0)
			imgui->applyAccentColor(config.accentColor);

		if (config.baseTheme != imgui->getBaseTheme())
			imgui->setBaseTheme(config.baseTheme);

		if ((windowState.closing || windowState.resetting) && !editor->isUpToDate() &&
		    !unsavedChangesDialog.open)
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
				case DialogResult::Yes:
					editor->trySave(editor->getWorkingFilename().data());
					glfwSetWindowShouldClose(window, 1);
					break;

				case DialogResult::No:
					glfwSetWindowShouldClose(window, 1);
					break;

				case DialogResult::Cancel:
					windowState.closing = false;
					break;

				default:
					break;
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
				case DialogResult::Yes:
					editor->trySave(editor->getWorkingFilename().data());
					break;

				case DialogResult::Cancel:
					windowState.resetting = shouldPickScore = false;
					pendingLoadScoreFile.clear();
					break;

				default:
					break;
				}
			}

			// Already saved or clicked save changes or discard changes
			if (editor->isUpToDate() || (unsavedChangesResult != DialogResult::Cancel &&
			                             unsavedChangesResult != DialogResult::None))
			{
				if (windowState.shouldPickScore)
				{
					editor->open();
					windowState.shouldPickScore = false;
				}
				else if (pendingLoadScoreFile.size())
				{
					editor->loadScore(pendingLoadScoreFile);
					pendingLoadScoreFile.clear();
				}
				else
				{
					editor->create();
				}

				windowState.resetting = false;
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
		ResourceManager::loadShader(appDir + "res\\shaders\\basic2d");
		const std::string texturesDir = appDir + "res\\textures\\";
		ResourceManager::loadTexture(texturesDir + "notes1.png",
		                             TextureFilterMode::LinearMipMapLinear,
		                             TextureFilterMode::Linear);
		ResourceManager::loadTexture(texturesDir + "notes2.png");
		ResourceManager::loadTexture(texturesDir + "longNoteLine.png");
		ResourceManager::loadTexture(texturesDir + "touchLine_eff.png");
		ResourceManager::loadTexture(texturesDir + "guideColors.png");

		ResourceManager::loadTexture(texturesDir + "timeline_select.png");
		ResourceManager::loadTexture(texturesDir + "timeline_tap.png");
		ResourceManager::loadTexture(texturesDir + "timeline_hold.png");
		ResourceManager::loadTexture(texturesDir + "timeline_hold_step_normal.png");
		ResourceManager::loadTexture(texturesDir + "timeline_hold_step_hidden.png");
		ResourceManager::loadTexture(texturesDir + "timeline_hold_step_skip.png");
		ResourceManager::loadTexture(texturesDir + "timeline_flick_default.png");
		ResourceManager::loadTexture(texturesDir + "timeline_flick_left.png");
		ResourceManager::loadTexture(texturesDir + "timeline_flick_right.png");
		ResourceManager::loadTexture(texturesDir + "timeline_critical.png");
		ResourceManager::loadTexture(texturesDir + "timeline_trace.png");
		for (auto color : guideColors)
			for (auto fade : fadeTypes)
				ResourceManager::loadTexture(
				    appDir + IO::formatString("res\\textures\\timeline_guide_%s_%s.png", color,
				                              std::string(fade).substr(5).c_str()));
		ResourceManager::loadTexture(texturesDir + "timeline_damage.png");
		ResourceManager::loadTexture(texturesDir + "timeline_bpm.png");
		ResourceManager::loadTexture(texturesDir + "timeline_time_signature.png");
		ResourceManager::loadTexture(texturesDir + "timeline_hi_speed.png");
		// Cache note textures indices
		noteTextures.notes = ResourceManager::getTexture(NOTES_TEX);
		noteTextures.holdPath = ResourceManager::getTexture(HOLD_PATH_TEX);
		noteTextures.touchLine = ResourceManager::getTexture(TOUCH_LINE_TEX);
		noteTextures.ccNotes = ResourceManager::getTexture(CC_NOTES_TEX);
		noteTextures.guideColors = ResourceManager::getTexture(GUIDE_COLORS_TEX);

		// Load more languages here
		Localization::loadDefault();
		Localization::load("ja", u8"日本語", appDir + "res\\i18n\\ja.csv");
	}

	void Application::run()
	{
		HWND hwnd = glfwGetWin32Window(window);

		/*
		    Override the current GLFW/Imgui window procedure and store it in the GLFW window user
		   pointer

		    NOTE: For this to be safe, it should be only called AFTER ImGui is initialized
		    so that the WndProc ImGui is expecting matches with our own WndProc
		*/
		glfwSetWindowUserPointer(window, (void*)::GetWindowLongPtrW(hwnd, GWLP_WNDPROC));
		::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);

		windowState.windowHandle = hwnd;
		windowState.windowTimerId =
		    ::SetTimer(hwnd, reinterpret_cast<UINT_PTR>(&windowState.windowTimerId),
		               USER_TIMER_MINIMUM, nullptr);

		::DragAcceptFiles(hwnd, TRUE);

		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
			update();
		}

		editor->savePresets(appDir + "library");
		writeSettings();
	}
}
