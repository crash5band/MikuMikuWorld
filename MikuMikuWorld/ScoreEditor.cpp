#include "Application.h"
#include "ApplicationConfiguration.h"
#include "File.h"
#include "SUS.h"
#include "SusExporter.h"
#include "SusParser.h"
#include "ScoreConverter.h"
#include "UI.h"
#include "Constants.h"
#include "Utilities.h"
#include "ImageCrop.h"
#include "ScoreSerializer.h"
#include <filesystem>
#include <Windows.h>

namespace MikuMikuWorld
{
	static MultiInputBinding* timelineModeBindings[] =
	{
		&config.input.timelineSelect,
		&config.input.timelineTap,
		&config.input.timelineHold,
		&config.input.timelineHoldMid,
		&config.input.timelineFlick,
		&config.input.timelineCritical,
		&config.input.timelineFriction,
		&config.input.timelineGuide,
		&config.input.timelineBpm,
		&config.input.timelineTimeSignature,
		&config.input.timelineHiSpeed,
	};

	ScoreEditor::ScoreEditor() : presetManager(Application::getAppDir() + "library")
	{
		renderer = std::make_unique<Renderer>();

		context.audio.initializeAudioEngine();
		context.audio.setMasterVolume(config.masterVolume);
		context.audio.setMusicVolume(config.bgmVolume);
		context.audio.setSoundEffectsVolume(config.seVolume);
		context.audio.loadSoundEffects();
		context.audio.setSoundEffectsProfileIndex(config.seProfileIndex);

		timeline.setDivision(config.division);
		timeline.setZoom(config.zoom);

		autoSavePath = Application::getAppDir() + "auto_save";
		autoSaveTimer.reset();
	}

	void ScoreEditor::writeSettings()
	{
		config.masterVolume = context.audio.getMasterVolume();
		config.bgmVolume = context.audio.getMusicVolume();
		config.seVolume = context.audio.getSoundEffectsVolume();

		config.division = timeline.getDivision();
		config.zoom = timeline.getZoom();
	}

	void ScoreEditor::uninitialize()
	{
		context.audio.uninitializeAudioEngine();
		timeline.background.dispose();
	}

	void ScoreEditor::update()
	{
		drawMenubar();
		drawToolbar();

		if (!ImGui::GetIO().WantCaptureKeyboard)
		{
			if (ImGui::IsAnyPressed(config.input.create)) Application::windowState.resetting = true;
			if (ImGui::IsAnyPressed(config.input.open))
			{
				Application::windowState.resetting = true;
				Application::windowState.shouldPickScore = true;
			}

			if (ImGui::IsAnyPressed(config.input.openSettings)) settingsWindow.open = true;
			if (ImGui::IsAnyPressed(config.input.openHelp)) help();
			if (ImGui::IsAnyPressed(config.input.save)) trySave(context.workingData.filename);
			if (ImGui::IsAnyPressed(config.input.saveAs)) saveAs();
			if (ImGui::IsAnyPressed(config.input.exportExternal)) exportExternal();
			if (ImGui::IsAnyPressed(config.input.togglePlayback)) timeline.setPlaying(context, !timeline.isPlaying());
			if (ImGui::IsAnyPressed(config.input.stop)) timeline.stop(context);
			if (ImGui::IsAnyPressed(config.input.previousTick, true)) timeline.previousTick(context);
			if (ImGui::IsAnyPressed(config.input.nextTick, true)) timeline.nextTick(context);
			if (ImGui::IsAnyPressed(config.input.selectAll)) context.selectAll();
			if (ImGui::IsAnyPressed(config.input.deleteSelection)) context.deleteSelection();
			if (ImGui::IsAnyPressed(config.input.cutSelection)) context.cutSelection();
			if (ImGui::IsAnyPressed(config.input.copySelection)) context.copySelection();
			if (ImGui::IsAnyPressed(config.input.paste)) context.paste(false);
			if (ImGui::IsAnyPressed(config.input.flipPaste)) context.paste(true);
			if (ImGui::IsAnyPressed(config.input.cancelPaste)) context.cancelPaste();
			if (ImGui::IsAnyPressed(config.input.flip)) context.flipSelection();
			if (ImGui::IsAnyPressed(config.input.undo)) context.undo();
			if (ImGui::IsAnyPressed(config.input.redo)) context.redo();
			if (ImGui::IsAnyPressed(config.input.zoomOut, true)) timeline.setZoom(timeline.getZoom() - 0.25f);
			if (ImGui::IsAnyPressed(config.input.zoomIn, true)) timeline.setZoom(timeline.getZoom() + 0.25f);
			if (ImGui::IsAnyPressed(config.input.decreaseNoteSize, true)) edit.noteWidth = std::clamp(edit.noteWidth - 1, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH);
			if (ImGui::IsAnyPressed(config.input.increaseNoteSize, true)) edit.noteWidth = std::clamp(edit.noteWidth + 1, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH);
			if (ImGui::IsAnyPressed(config.input.shrinkDown)) context.shrinkSelection(Direction::Down);
			if (ImGui::IsAnyPressed(config.input.shrinkUp)) context.shrinkSelection(Direction::Up);
			if (ImGui::IsAnyPressed(config.input.connectHolds)) context.connectHoldsInSelection();
			if (ImGui::IsAnyPressed(config.input.splitHold)) context.splitHoldInSelection();

			for (int i = 0; i < (int)TimelineMode::TimelineModeMax; ++i)
				if (ImGui::IsAnyPressed(*timelineModeBindings[i])) timeline.changeMode((TimelineMode)i, edit);
		}

		if (config.matchTimelineSizeToScreen)
		{
			int laneWidth = static_cast<int>(timeline.getWidth() * 0.45f / NUM_LANES);
			timeline.laneWidth = std::clamp(laneWidth, MIN_LANE_WIDTH, MAX_LANE_WIDTH);
		}
		else
		{
			timeline.laneWidth = config.timelineWidth;
		}

		timeline.notesHeight = config.matchNotesSizeToTimeline ? std::clamp(static_cast<int>(timeline.laneWidth), MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT) : config.notesHeight;

		if (config.backgroundBrightness != timeline.background.getBrightness())
			timeline.background.setBrightness(config.backgroundBrightness);

		if (settingsWindow.isBackgroundChangePending)
		{
			static const std::string defaultBackgroundPath = Application::getAppDir() + "res\\textures\\default.png";
			timeline.background.load(config.backgroundImage.empty() ? defaultBackgroundPath : config.backgroundImage);
			settingsWindow.isBackgroundChangePending = false;
		}

		if (config.seProfileIndex != context.audio.getSoundEffectsProfileIndex())
		{
			context.audio.stopSoundEffects(false);
			context.audio.setSoundEffectsProfileIndex(config.seProfileIndex);
		}

		if (propertiesWindow.isPendingLoadMusic)
		{
			asyncLoadMusic(propertiesWindow.pendingLoadMusicFilename);
			propertiesWindow.isPendingLoadMusic = false;
			propertiesWindow.pendingLoadMusicFilename.clear();
		}

		if (config.autoSaveEnabled && autoSaveTimer.elapsedMinutes() >= config.autoSaveInterval)
		{
			autoSave();
			autoSaveTimer.reset();
		}

		if (recentFileNotFoundDialog.update() == DialogResult::Yes)
		{
			if (isArrayIndexInBounds(recentFileNotFoundDialog.removeIndex, config.recentFiles))
				config.recentFiles.erase(config.recentFiles.begin() + recentFileNotFoundDialog.removeIndex);
		}

		settingsWindow.update();
		aboutDialog.update();
		if (serializationDialog)
		{
			switch (serializationDialog->update())
			{
			case SerializationDialogResult::SerializeSuccess:
				IO::messageBox(
					APP_NAME,
					IO::formatString(getString("export_successful")),
					IO::MessageBoxButtons::Ok,
					IO::MessageBoxIcon::Information,
					Application::windowState.windowHandle
				);
				serializationDialog.reset();
				break;
			case SerializationDialogResult::DeserializeSuccess:
				context.clearSelection();
				context.history.clear();
				context.score = serializationDialog->getScore();
				context.workingData = EditorScoreData(context.score.metadata, serializationDialog->getFilename());

				asyncLoadMusic(context.workingData.musicFilename);
				context.audio.setMusicOffset(0, context.workingData.musicOffset);

				context.scoreStats.calculateStats(context.score);
				context.scorePreviewDrawData.calculateDrawData(context.score);
				timeline.calculateMaxOffsetFromScore(context.score);

				UI::setWindowTitle((context.workingData.filename.size() ? IO::File::getFilename(context.workingData.filename) : windowUntitled));
				context.upToDate = true;
				serializationDialog.reset();
				break;
			default:
			case SerializationDialogResult::Error:
				IO::messageBox(
					APP_NAME,
					serializationDialog->getErrorMessage(),
					IO::MessageBoxButtons::Ok,
					IO::MessageBoxIcon::Error,
					Application::windowState.windowHandle
				);
				serializationDialog.reset();
				break;
			case SerializationDialogResult::None:
				break;
			}
		}

		const bool timeline_just_created = (ImGui::FindWindowByName("###notes_timeline") == NULL);
		ImGui::Begin(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline"), NULL, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGuiID dockId = ImGui::GetWindowDockID();
		timeline.update(context, edit, renderer.get());
		ImGui::End();

		ImGui::SetNextWindowDockID(dockId, ImGuiCond_FirstUseEver);
		ImGui::Begin(IMGUI_TITLE(ICON_FA_OBJECT_GROUP, "score_preview"), NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		preview.update(context, renderer.get());
		preview.updateUI(timeline, context);
		ImGui::End();

		if (timeline_just_created) ImGui::SetWindowFocus("###notes_timeline");

		if (config.debugEnabled)
		{
			debugWindow.update(context, timeline);
		}

		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_ALIGN_LEFT, "chart_properties"), NULL, ImGuiWindowFlags_Static))
		{
			propertiesWindow.update(context);
		}
		ImGui::End();

		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_WRENCH, "options"), NULL, ImGuiWindowFlags_Static))
		{
			optionsWindow.update(context, edit, timeline.getMode());
		}
		ImGui::End();

		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_DRAFTING_COMPASS, "presets"), NULL, ImGuiWindowFlags_Static))
		{
			presetsWindow.update(context, presetManager);
		}
		ImGui::End();

#ifdef _DEBUG
		if (showImGuiDemoWindow)
			ImGui::ShowDemoWindow(&showImGuiDemoWindow);
#endif
	}

	size_t ScoreEditor::updateRecentFilesList(const std::string& entry)
	{
		while (config.recentFiles.size() >= maxRecentFilesEntries)
			config.recentFiles.pop_back();

		// Remove the entry (if found) to the beginning of the vector
		auto it = std::find(config.recentFiles.begin(), config.recentFiles.end(), entry);
		if (it != config.recentFiles.end())
			config.recentFiles.erase(it);

		config.recentFiles.insert(config.recentFiles.begin(), entry);
		return config.recentFiles.size();
	}

	void ScoreEditor::reset()
	{
		timeline.setPlaying(context, false);

		context.score = {};
		context.workingData = {};
		context.history.clear();
		context.scoreStats.reset();
		context.scorePreviewDrawData.clear();
		context.audio.disposeMusic();
		context.waveformL.clear();
		context.waveformR.clear();
		context.clearSelection();

		// New score; nothing to save
		context.upToDate = true; 

		UI::setWindowTitle(windowUntitled);
	}

	void ScoreEditor::loadScore(std::string filename)
	{
		if (!IO::File::exists(filename))
			return;

		timeline.setPlaying(context, false);
		serializationDialog = SerializationDialogFactory::getDialog(filename);
		serializationDialog->openDeserializingDialog(filename);

		updateRecentFilesList(filename);
	}

	void ScoreEditor::loadMusic(std::string filename)
	{
		propertiesWindow.isLoadingMusic = true;
		context.waveformL.clear();
		context.waveformR.clear();
		
		Result result = context.audio.loadMusic(filename);
		if (result.isOk() || filename.empty())
		{
			context.workingData.musicFilename = filename;
		}
		else
		{
			std::string errorMessage = IO::formatString("%s\n%s: %s\n%s",
				getString("error_load_music_file"),
				getString("music_file"),
				filename.c_str(),
				result.getMessage().c_str()
			);

			IO::messageBox(APP_NAME, errorMessage, IO::MessageBoxButtons::Ok, IO::MessageBoxIcon::Error, Application::windowState.windowHandle);
		}
		
		context.waveformL.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 0);
		context.waveformR.generateMipChainsFromSampleBuffer(context.audio.musicBuffer, 1);

		if (timeline.isPlaying())
		{
			bool prevPauseBehaviour = config.returnToLastSelectedTickOnPause;
			config.returnToLastSelectedTickOnPause = false;
			timeline.setPlaying(context, false);
			timeline.setPlaying(context, true);

			config.returnToLastSelectedTickOnPause = prevPauseBehaviour;
		}

		propertiesWindow.isLoadingMusic = false;
	}

	void ScoreEditor::asyncLoadMusic(std::string filename)
	{
		if (loadMusicFuture.valid())
			loadMusicFuture.get();
		
		loadMusicFuture = std::async(&ScoreEditor::loadMusic, this, filename);
	}

	void ScoreEditor::open()
	{
		IO::FileDialog fileDialog{};
		fileDialog.parentWindowHandle = Application::windowState.windowHandle;
		fileDialog.title = "Open Chart";
		fileDialog.filters = { IO::combineFilters("All Supported Files", {IO::mmwsFilter, IO::susFilter, IO::scpFilter}) };
		
		if (fileDialog.openFile() == IO::FileDialogResult::OK)
			loadScore(fileDialog.outputFilename);
	}

	bool ScoreEditor::trySave(std::string filename)
	{
		return filename.empty() ? saveAs() : save(filename);
	}

	bool ScoreEditor::save(std::string filename)
	{
		try
		{
			context.score.metadata = context.workingData.toScoreMetadata();
			ScoreSerializerFactory::getSerializer(MMWS_EXTENSION)->serialize(context.score, filename);

			UI::setWindowTitle(IO::File::getFilename(filename));
			context.upToDate = true;
		}
		catch (const std::exception& err)
		{
			IO::messageBox(
				APP_NAME,
				IO::formatString("An error occured while saving the score file\n%s", err.what()),
				IO::MessageBoxButtons::Ok,
				IO::MessageBoxIcon::Error,
				Application::windowState.windowHandle
			);

			return false;
		}

		return true;
	}

	bool ScoreEditor::saveAs()
	{
		IO::FileDialog fileDialog{};
		fileDialog.title = "Save Chart";
		fileDialog.filters = { IO::mmwsFilter };
		fileDialog.defaultExtension = "mmws";
		fileDialog.parentWindowHandle = Application::windowState.windowHandle;
		fileDialog.inputFilename = IO::File::getFilenameWithoutExtension(context.workingData.filename);

		if (fileDialog.saveFile() == IO::FileDialogResult::OK)
		{
			context.workingData.filename = fileDialog.outputFilename;
			bool saved = save(context.workingData.filename);
			if (saved)
				updateRecentFilesList(fileDialog.outputFilename);

			return saved;
		}

		return false;
	}

	void ScoreEditor::exportExternal()
	{
		constexpr const char* exportExtensions[] = { "sus", "scp" };
		int filterIndex = std::clamp(config.lastSelectedExportIndex, 0, static_cast<int>(arrayLength(exportExtensions) - 1));

		IO::FileDialog fileDialog{};
		fileDialog.title = "Export Chart";
		fileDialog.filters = { IO::susFilter, IO::scpFilter };
		fileDialog.filterIndex = filterIndex;
		fileDialog.defaultExtension = exportExtensions[filterIndex];
		fileDialog.parentWindowHandle = Application::windowState.windowHandle;

		if (fileDialog.saveFile() == IO::FileDialogResult::OK)
		{
			serializationDialog = SerializationDialogFactory::getDialog(fileDialog.outputFilename);
			serializationDialog->openSerializingDialog(context, fileDialog.outputFilename);
			
			config.lastSelectedExportIndex = fileDialog.filterIndex;
		}
	}

	void ScoreEditor::drawMenubar()
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 2));

		if (ImGui::BeginMenu(getString("file")))
		{
			if (ImGui::MenuItem(getString("new"), ToShortcutString(config.input.create)))
				Application::windowState.resetting = true;

			if (ImGui::MenuItem(getString("open"), ToShortcutString(config.input.open)))
			{
				Application::windowState.resetting = true;
				Application::windowState.shouldPickScore = true;
			}

			if (ImGui::BeginMenu(getString("open_recent")))
			{
				for (size_t index = 0; index < config.recentFiles.size(); index++)
				{
					const std::string& entry = config.recentFiles[index];
					if (ImGui::MenuItem(entry.c_str()))
					{
						if (IO::File::exists(entry))
						{
							Application::windowState.resetting = true;
							Application::pendingLoadScoreFile = entry;
						}
						else
						{
							recentFileNotFoundDialog.removeFilename = entry;
							recentFileNotFoundDialog.removeIndex = index;
							recentFileNotFoundDialog.open = true;
						}
					}
				}

				ImGui::Separator();
				if (ImGui::MenuItem(getString("clear"), nullptr, false, !config.recentFiles.empty()))
					config.recentFiles.clear();

				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::MenuItem(getString("save"), ToShortcutString(config.input.save)))
				trySave(context.workingData.filename);

			if (ImGui::MenuItem(getString("save_as"), ToShortcutString(config.input.saveAs)))
				saveAs();

			if (ImGui::MenuItem(getString("export"), ToShortcutString(config.input.exportExternal)))
				exportExternal();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("exit"), ToShortcutString(ImGuiKey_F4, ImGuiModFlags_Alt)))
				Application::windowState.closing = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("edit")))
		{
			if (ImGui::MenuItem(getString("undo"), ToShortcutString(config.input.undo), false, context.history.hasUndo()))
				context.undo();

			if (ImGui::MenuItem(getString("redo"), ToShortcutString(config.input.redo), false, context.history.hasRedo()))
				context.redo();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("delete"), ToShortcutString(config.input.deleteSelection), false, context.selectedNotes.size()))
				context.deleteSelection();

			if (ImGui::MenuItem(getString("cut"), ToShortcutString(config.input.cutSelection), false, context.selectedNotes.size()))
				context.cutSelection();

			if (ImGui::MenuItem(getString("copy"), ToShortcutString(config.input.copySelection), false, context.selectedNotes.size()))
				context.copySelection();

			if (ImGui::MenuItem(getString("paste"), ToShortcutString(config.input.paste)))
				context.paste(false);

			ImGui::Separator();
			if (ImGui::MenuItem(getString("select_all"), ToShortcutString(config.input.selectAll)))
				context.selectAll();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("settings"), ToShortcutString(config.input.openSettings)))
				settingsWindow.open = true;

			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu(getString("view")))
		{
			ImGui::MenuItem(getString("show_step_outlines"), NULL, &timeline.drawHoldStepOutlines);
			ImGui::MenuItem(getString("hide_step_outlines_in_playback"), NULL, &config.hideStepOutlinesInPlayback);
			ImGui::MenuItem(getString("cursor_auto_scroll"), NULL, &config.followCursorInPlayback);
			ImGui::MenuItem(getString("return_to_last_tick"), NULL, &config.returnToLastSelectedTickOnPause);
			ImGui::MenuItem(getString("draw_waveform"), NULL, &config.drawWaveform);
			ImGui::MenuItem(getString("stop_at_end"), NULL, &config.stopAtEnd);
			ImGui::MenuItem(getString("preview_draw_toolbar"), NULL, &config.pvDrawToolbar);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("presets")))
		{
			if (ImGui::MenuItem(getString("create_preset"), NULL, false, !context.selectedNotes.empty()))
				presetsWindow.openCreatePresetDialog();
			
			if (ImGui::MenuItem(getString("import_preset"), NULL, false, true))
				openImportPresetDialog();
			
			if (ImGui::MenuItem(getString("reload_presets"), NULL, false, true))
			{
				presetManager.presets.clear();
				loadPresets();
			}
			
			ImGui::Separator();
			if (ImGui::MenuItem(getString("open_presets_folder"), NULL, false, true))
			{
				if (!std::filesystem::exists(presetManager.getPresetsPath()))
					std::filesystem::create_directory(presetManager.getPresetsPath());
				ShellExecuteW(0, 0, presetManager.getPresetsPath().data(), 0, 0, SW_SHOW);
			}
			
			ImGui::EndMenu();
		}
		
		if (config.debugEnabled)
		{
			if (ImGui::BeginMenu(getString("debug")))
			{
#ifdef _DEBUG
				ImGui::MenuItem("ImGui Demo Window", NULL, &showImGuiDemoWindow);
#endif

				if (ImGui::MenuItem("Auto Save"))
					autoSave();

				if (ImGui::MenuItem("Delete Old Auto Save (1)"))
					deleteOldAutoSave(1);

				if (ImGui::MenuItem("Delete Old Auto Save (Max)"))
					deleteOldAutoSave(config.autoSaveMaxCount);

				bool audioRunning = context.audio.isEngineStarted();
				if (ImGui::MenuItem(audioRunning ? "Stop Audio" : "Start Audio",
					audioRunning ? ICON_FA_VOLUME_UP : ICON_FA_VOLUME_MUTE))
				{
					if (audioRunning)
						context.audio.stopEngine();
					else
						context.audio.startEngine();
				}

				ImGui::EndMenu();
			}
		}

		if (ImGui::BeginMenu(getString("window")))
		{
			if (ImGui::MenuItem(getString("vsync"), NULL, &Application::windowState.vsync))
				glfwSwapInterval(Application::windowState.vsync);

			ImGui::MenuItem(getString("show_fps"), NULL, &config.showFPS);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("help")))
		{
			if (ImGui::MenuItem(getString("help"), ToShortcutString(config.input.openHelp)))
				help();

			if (ImGui::MenuItem(getString("about")))
				aboutDialog.open = true;

			ImGui::EndMenu();
		}

		if (config.showFPS)
		{
			std::string fps = IO::formatString("%.3fms (%.1fFPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::SetCursorPosX(ImGui::GetWindowSize().x - ImGui::CalcTextSize(fps.c_str()).x - ImGui::GetStyle().WindowPadding.x);
			ImGui::Text(fps.c_str());
		}

		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}

	void ScoreEditor::drawToolbar()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 toolbarSize{ viewport->WorkSize.x, UI::toolbarBtnSize.y + ImGui::GetStyle().WindowPadding.y + 5 };

		// keep toolbar on top in main viewport
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(toolbarSize, ImGuiCond_Always);

		// toolbar style
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("##app_toolbar", NULL, ImGuiWindowFlags_Toolbar);
		
		if (UI::toolbarButton(ICON_FA_FILE, getString("new"), ToShortcutString(config.input.create)))
		{
			Application::windowState.resetting = true;
		}

		if (UI::toolbarButton(ICON_FA_FOLDER_OPEN, getString("open"), ToShortcutString(config.input.open)))
		{
			Application::windowState.resetting = true;
			Application::windowState.shouldPickScore = true;
		}

		if (UI::toolbarButton(ICON_FA_SAVE, getString("save"), ToShortcutString(config.input.save)))
			trySave(context.workingData.filename);

		if (UI::toolbarButton(ICON_FA_FILE_EXPORT, getString("export"), ToShortcutString(config.input.exportExternal)))
			exportExternal();

		UI::toolbarSeparator();

		if (UI::toolbarButton(ICON_FA_CUT, getString("cut"), ToShortcutString(config.input.cutSelection), context.selectedNotes.size() > 0))
			context.cutSelection();

		if (UI::toolbarButton(ICON_FA_COPY, getString("copy"), ToShortcutString(config.input.copySelection), context.selectedNotes.size() > 0))
			context.copySelection();

		if (UI::toolbarButton(ICON_FA_PASTE, getString("paste"), ToShortcutString(config.input.paste)))
			context.paste(false);

		UI::toolbarSeparator();

		if (UI::toolbarButton(ICON_FA_UNDO, getString("undo"), ToShortcutString(config.input.undo), context.history.hasUndo()))
			context.undo();

		if (UI::toolbarButton(ICON_FA_REDO, getString("redo"), ToShortcutString(config.input.redo), context.history.hasRedo()))
			context.redo();

		UI::toolbarSeparator();

		for (int i = 0; i < (int)TimelineMode::TimelineModeMax; ++i)
		{
			std::string img{ IO::concat("timeline_tools", timelineModes[i], "_") };
			int sprIndex = (int)timelineSprites[i];
			if (i == (int)TimelineMode::InsertFlick)
			{
				img.append(IO::concat("", flickTypes[(int)edit.flickType], "_"));
				sprIndex = (int)flickToolSprites[(int)edit.flickType - 1];
			}
			else if (i == (int)TimelineMode::InsertLongMid)
			{
				img.append(IO::concat("", stepTypes[(int)edit.stepType], "_"));
				sprIndex = (int)stepToolSprites[(int)edit.stepType];
			}

			if (UI::toolbarImageButton("timeline_tools", sprIndex, getString(timelineModes[i]), ToShortcutString(*timelineModeBindings[i]), true, (int)timeline.getMode() == i))
				timeline.changeMode((TimelineMode)i, edit);
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		ImGui::End();
	}

	void ScoreEditor::help()
	{
		ShellExecuteW(0, 0, L"https://github.com/crash5band/MikuMikuWorld/wiki", 0, 0, SW_SHOW);
	}

	void ScoreEditor::autoSave()
	{
		std::wstring wAutoSaveDir = IO::mbToWideStr(autoSavePath);
		std::filesystem::create_directory(wAutoSaveDir);

		context.score.metadata = context.workingData.toScoreMetadata();
		ScoreSerializerFactory::getSerializer(MMWS_EXTENSION)->serialize(context.score, autoSavePath + "\\mmw_auto_save_" + Utilities::getCurrentDateTime() + MMWS_EXTENSION);
		
		int mmwsCount = 0;
		for (const auto& file : std::filesystem::directory_iterator(wAutoSaveDir))
		{
			std::string extension = file.path().extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
			mmwsCount += static_cast<int>(extension == MMWS_EXTENSION);
		}
		
		if (mmwsCount > config.autoSaveMaxCount)
			deleteOldAutoSave(mmwsCount - config.autoSaveMaxCount);
	}

	int ScoreEditor::deleteOldAutoSave(int count)
	{
		std::wstring wAutoSaveDir = IO::mbToWideStr(autoSavePath);
		if (!std::filesystem::exists(wAutoSaveDir))
			return 0;

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
		std::sort(deleteFiles.begin(), deleteFiles.end(), [](const entry& f1, const entry& f2) {
			return f1.last_write_time() < f2.last_write_time();
		});

		int deleteCount = std::min(static_cast<int>(deleteFiles.size()), count);
		for (int i = 0; i < deleteCount; i++)
			std::filesystem::remove(deleteFiles[i]);
		
		return deleteCount;
	}

	void ScoreEditor::loadPresets()
	{
		if (loadPresetsFuture.valid())
			loadPresetsFuture.get();
		
		loadPresetsFuture = std::async(std::launch::async, [this]()
		{
			presetsWindow.loadingPresets = true;
			presetManager.loadPresets();
			presetsWindow.loadingPresets = false;
		});
	}

	void ScoreEditor::importPreset(const std::string& path)
	{
		if (importPresetFuture.valid())
			importPresetFuture.get();

		presetsWindow.loadingPresets = true;
		importPresetFuture = std::async(std::launch::async, [this, path]()
		{
			Result result = presetManager.importPreset(path);
			presetsWindow.loadingPresets = false;

			if (result.getStatus() != ResultStatus::Success)
			{
				IO::MessageBoxIcon icon = result.getStatus() == ResultStatus::Error ?
					IO::MessageBoxIcon::Error :
					IO::MessageBoxIcon::Warning;
			
				IO::messageBox(APP_NAME, result.getMessage(), IO::MessageBoxButtons::Ok, icon, Application::windowState.windowHandle);
			}
		});
	}

	void ScoreEditor::openImportPresetDialog()
	{
		IO::FileDialog fileDialog{"Import Preset", { IO::presetFilter }, "", "", ".json", 0, Application::windowState.windowHandle};
		if (fileDialog.openFile() == IO::FileDialogResult::OK)
			importPreset(fileDialog.outputFilename);
	}
}