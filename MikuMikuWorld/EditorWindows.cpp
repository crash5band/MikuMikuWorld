#include "ScoreEditor.h"
#include "Utilities.h"
#include "ResourceManager.h"
#include "InputListener.h"
#include "IconsFontAwesome5.h"
#include "Score.h"
#include "ImGui/imgui_stdlib.h"
#include "FileDialog.h"
#include "UI.h"
#include "Stopwatch.h"
#include "HistoryManager.h"
#include "StringOperations.h"
#include "Tempo.h"
#include "Colors.h"
#include "Clipboard.h"
#include "Localization.h"
#include "Commands/CommandManager.h"
#include <algorithm>

namespace MikuMikuWorld
{
	void ScoreEditor::toolboxWindow()
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_TOOLBOX, "toolbox")))
		{
			ImVec2 btnSz{ ImGui::GetContentRegionAvail().x, 35.0f };
			for (int i = 0; i < (int)TimelineMode::TimelineToolMax; ++i)
			{
				ImGui::PushID(i);
				bool highlight = (int)currentMode == i;
				if (highlight)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
				}

				std::string str = getString(timelineModes[i]);
				if (ImGui::Button(str.c_str(), btnSz))
					changeMode((TimelineMode)i);

				if (highlight)
					ImGui::PopStyleColor(2);

				ImGui::PopID();
			}

			ImGui::Separator();
			UI::beginPropertyColumns();

			if (currentMode == TimelineMode::InsertBPM)
			{
				UI::addFloatProperty(getString("bpm"), defaultBPM, "%g BPM");
				defaultBPM = std::clamp(defaultBPM, MIN_BPM, MAX_BPM);
			}
			else if (currentMode == TimelineMode::InsertTimeSign)
			{
				UI::timeSignatureSelect(defaultTimeSignN, defaultTimeSignD);
			}
			else
			{
				UI::addIntProperty(getString("note_width"), defaultNoteWidth, 1, 12);
				UI::addSelectProperty(getString("step_type"), defaultStepType, uiStepTypes, TXT_ARR_SZ(uiStepTypes));
			}

			UI::endPropertyColumns();
		}

		ImGui::End();
	}
	
	void ScoreEditor::updateTimeline(float frameTime, Renderer* renderer, CommandManager* commandManager)
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline"),
			NULL, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			windowFocused = ImGui::IsWindowFocused();
			if (canvas.isMouseInCanvas() && !UI::isAnyPopupOpen())
			{
				// get mouse position relative to canvas
				mousePos = ImGui::GetIO().MousePos - canvas.getPosition();
				mousePos.y -= canvas.getOffset();

				if (!isHoveringNote && !isHoldingNote && !insertingHold)
				{
					if (mouseClickedOnTimeline && ImGui::IsMouseDown(0) && ImGui::IsMouseDragPastThreshold(0, 10.0f))
						dragging = currentMode == TimelineMode::Select;

					if (ImGui::IsMouseClicked(0))
					{
						if (!InputListener::isCtrlDown() && !InputListener::isAltDown() &&
							!ImGui::IsPopupOpen(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
							selection.clear();

						if (currentMode == TimelineMode::Select)
						{
							dragStart = mousePos;
							mouseClickedOnTimeline = true;
						}
					}
				}
			}

			canvas.update(frameTime);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->PushClipRect(canvas.getBoundaries().Min, canvas.getBoundaries().Max, true);

			canvas.updateScorllingPosition(frameTime);
			canvas.drawBackground(renderer);
			canvas.drawLanesBackground();
			
			drawMeasures();
			updateTempoChanges();
			updateTimeSignatures();
			bpmEditor();
			timeSignatureEditor();
			
			for (const auto& skill : score.skills)
				skillControl(skill);

			feverControl(score.fever);
			drawLanes();

			contextMenu(commandManager);
			updateCursor();

			noteCtrlHeight = canvas.getNotesHeight();
			isHoveringNote = false;
			hoveringNote = -1;
			minNoteYDistance = 9999.0;
			updateNotes(renderer);

			// update dragging
			if (dragging)
			{
				drawSelectionRectangle();
				if (ImGui::IsMouseReleased(0))
				{
					calcDragSelection();
					dragging = false;
				}
			}

			if (canvas.isMouseInCanvas() && (isPasting() || insertingPreset))
			{
				if (ImGui::IsMouseClicked(0))
					confirmPaste();
				else if (ImGui::IsMouseClicked(1))
					cancelPaste();
			}

			if (ImGui::IsMouseReleased(0))
				mouseClickedOnTimeline = false;

			selection.update(score);
			drawList->PopClipRect();
		}
		ImGui::End();
	}

	void ScoreEditor::contextMenu(CommandManager* commandManager)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 8));
		if (ImGui::BeginPopupContextWindow(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
		{
			int deleteCmd = commandManager->findCommand("delete");
			int copyCmd = commandManager->findCommand("copy");
			int cutCmd = commandManager->findCommand("cut");
			int pasteCmd = commandManager->findCommand("paste");
			int flipPasteCmd = commandManager->findCommand("flip_paste");
			int flipCmd = commandManager->findCommand("flip");
			int shrinkDownCmd = commandManager->findCommand("shrink_down");
			int shrinkUpCmd = commandManager->findCommand("shrink_up");

			UI::contextMenuItem(ICON_FA_TRASH, commandManager->commands[deleteCmd]);

			ImGui::Separator();
			UI::contextMenuItem(ICON_FA_CUT, commandManager->commands[cutCmd]);
			UI::contextMenuItem(ICON_FA_COPY, commandManager->commands[copyCmd]);
			UI::contextMenuItem(ICON_FA_PASTE, commandManager->commands[pasteCmd]);

			ImGui::Separator();
			UI::contextMenuItem(ICON_FA_PASTE, commandManager->commands[flipPasteCmd]);
			UI::contextMenuItem(ICON_FA_GRIP_LINES_VERTICAL, commandManager->commands[flipCmd]);

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("ease_type"), selection.hasEase()))
			{
				for (int i = 0; i < TXT_ARR_SZ(uiEaseTypes); ++i)
				{
					if (ImGui::MenuItem(getString(uiEaseTypes[i])))
						setEase((EaseType)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("step_type"), selection.hasStep()))
			{
				for (int i = 0; i < TXT_ARR_SZ(uiStepTypes); ++i)
				{
					if (ImGui::MenuItem(getString(uiStepTypes[i])))
						setStepType((HoldStepType)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("flick_type"), selection.hasFlickable()))
			{
				for (int i = 0; i < TXT_ARR_SZ(uiFlickTypes); ++i)
				{
					if (ImGui::MenuItem(getString(uiFlickTypes[i])))
						setFlick((FlickType)i);
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			UI::contextMenuItem(ICON_FA_ARROW_DOWN, commandManager->commands[shrinkDownCmd]);
			UI::contextMenuItem(ICON_FA_ARROW_UP, commandManager->commands[shrinkUpCmd]);

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

	void ScoreEditor::propertiesWindow()
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_ALIGN_LEFT, "chart_properties"), NULL, ImGuiWindowFlags_Static))
		{
			if (ImGui::CollapsingHeader(
				concat(ICON_FA_ALIGN_LEFT, getString("metadata"), " ").c_str(),
				ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				UI::addStringProperty(getString("title"), workingData.title);
				UI::addStringProperty(getString("designer"), workingData.designer);
				UI::addStringProperty(getString("artist"), workingData.artist);

				std::string jacketFile = workingData.jacket.getFilename();
				int result = UI::addFileProperty(getString("jacket"), jacketFile);
				if (result == 1)
				{
					workingData.jacket.load(jacketFile);
				}
				else if (result == 2)
				{
					std::string name;
					if (FileDialog::openFile(name, FileType::ImageFile))
						workingData.jacket.load(name);
				}
				workingData.jacket.draw();
				UI::endPropertyColumns();
			}

			if (ImGui::CollapsingHeader(
				concat(ICON_FA_VOLUME_UP, getString("audio"), " ").c_str(),
				ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				std::string filename = workingData.musicFilename;
				int filePickResult = UI::addFileProperty(getString("music_file"), filename);
				if (filePickResult == 1 && filename != workingData.musicFilename)
				{
					loadMusic(filename);
				}
				else if (filePickResult == 2)
				{
					if (FileDialog::openFile(filename, FileType::AudioFile) && filename != workingData.musicFilename)
						loadMusic(filename);
				}

				float offset = workingData.musicOffset;
				UI::addFloatProperty(getString("music_offset"), offset, "%.03fms");
				if (offset != workingData.musicOffset)
				{
					workingData.musicOffset = offset;
					audio.setBGMOffset(time, offset);
				}

				// volume controls
				float master = audio.getMasterVolume();
				float bgm = audio.getBGMVolume();
				float se = audio.getSEVolume();

				UI::addPercentSliderProperty(getString("volume_master"), master);
				UI::addPercentSliderProperty(getString("volume_bgm"), bgm);
				UI::addPercentSliderProperty(getString("volume_se"), se);
				UI::endPropertyColumns();

				if (master != audio.getMasterVolume())
					audio.setMasterVolume(master);

				if (bgm != audio.getBGMVolume())
					audio.setBGMVolume(bgm);

				if (se != audio.getSEVolume())
					audio.setSEVolume(se);
			}

			if (ImGui::CollapsingHeader(
				concat(ICON_FA_CHART_BAR, getString("statistics"), " ").c_str(),
				ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				UI::addReadOnlyProperty(getString("taps"), std::to_string(stats.getTaps()));
				UI::addReadOnlyProperty(getString("flicks"), std::to_string(stats.getFlicks()));
				UI::addReadOnlyProperty(getString("holds"), std::to_string(stats.getHolds()));
				UI::addReadOnlyProperty(getString("steps"), std::to_string(stats.getSteps()));
				UI::addReadOnlyProperty(getString("total"), std::to_string(stats.getTotal()));
				UI::addReadOnlyProperty(getString("combo"), std::to_string(stats.getCombo()));
				UI::endPropertyColumns();
			}
		}

		ImGui::End();
	}

	void ScoreEditor::controlsWindow()
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_ADJUST, "controls")))
		{
			int measure = accumulateMeasures(currentTick, TICKS_PER_BEAT, score.timeSignatures);
			const TimeSignature& ts = score.timeSignatures[findTimeSignature(measure, score.timeSignatures)];
			const Tempo& tempo = getTempoAt(currentTick, score.tempoChanges);

			Utilities::ImGuiCenteredText(formatString(
				"%02d:%02d:%02d\t|\t%d/%d\t|\t%g BPM",
				(int)time / 60, (int)time % 60, (int)((time - (int)time) * 100),
				ts.numerator, ts.denominator,
				tempo.bpm
			));

			// center playback controls
			ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x * 0.5f) - ((UI::btnNormal.x + ImGui::GetStyle().ItemSpacing.x) * 2));
			if (UI::transparentButton(ICON_FA_BACKWARD, UI::btnNormal, true, !playing))
				previousTick();

			ImGui::SameLine();
			if (UI::transparentButton(ICON_FA_STOP))
				stop();

			ImGui::SameLine();
			if (UI::transparentButton(playing ? ICON_FA_PAUSE : ICON_FA_PLAY))
				togglePlaying();

			ImGui::SameLine();
			if (UI::transparentButton(ICON_FA_FORWARD, UI::btnNormal, true, !playing))
				nextTick();

			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			
			UI::beginPropertyColumns();
			UI::divisionSelect(getString("division"), division, divisions, sizeof(divisions) / sizeof(int));

			static int m = 0;
			UI::propertyLabel(getString("goto_measure"));
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
			ImGui::InputInt("##goto_measure", &m, 0, 0);

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_ARROW_RIGHT, UI::btnSmall))
				gotoMeasure(m);
			ImGui::NextColumn();

			UI::addSelectProperty(getString("scroll_mode"), scrollMode, scrollModes, (int)ScrollMode::ScrollModeMax);
			
			float _zoom = canvas.getZoom();
			if (UI::zoomControl("zoom", _zoom, MIN_ZOOM, 10.0f))
				canvas.setZoom(_zoom);

			UI::endPropertyColumns();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Checkbox(getString("show_step_outlines"), &drawHoldStepOutline);
		}

		ImGui::End();
	}

	void ScoreEditor::bpmEditor()
	{
		ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
		if (ImGui::BeginPopup("edit_bpm"))
		{
			ImGui::Text(getString("edit_bpm"));
			ImGui::Separator();

			Tempo& tempo = score.tempoChanges[editBPMIndex];
			UI::beginPropertyColumns();
			UI::addReadOnlyProperty(getString("tick"), std::to_string(tempo.tick));
			UI::addFloatProperty(getString("bpm"), editBPM, "%g");
			UI::endPropertyColumns();

			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				Score prev = score;
				tempo.bpm = std::clamp(editBPM, MIN_BPM, MAX_BPM);

				pushHistory("Change tempo", prev, score);
			}

			// cannot remove the first tempo change
			if (tempo.tick != 0)
			{
				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = score;
					score.tempoChanges.erase(score.tempoChanges.begin() + editBPMIndex);
					pushHistory("Remove tempo change", prev, score);
				}
			}

			ImGui::EndPopup();
		}
	}

	void ScoreEditor::timeSignatureEditor()
	{
		ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
		if (ImGui::BeginPopup("edit_ts"))
		{
			ImGui::Text(getString("edit_time_signature"));
			ImGui::Separator();

			UI::beginPropertyColumns();
			UI::addReadOnlyProperty(getString("measure"), std::to_string(editTSIndex));
			if (UI::timeSignatureSelect(editTsNum, editTsDenom))
			{
				Score prev = score;
				TimeSignature& ts = score.timeSignatures[editTSIndex];
				ts.numerator = std::clamp(abs(editTsNum), MIN_TIME_SIGN, MAX_TIME_SIGN);
				ts.denominator = std::clamp(abs(editTsDenom), MIN_TIME_SIGN, MAX_TIME_SIGN);

				pushHistory("Change time signature", prev, score);
			}
			UI::endPropertyColumns();

			// cannot remove the first time signature
			if (editTSIndex != 0)
			{
				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = score;
					score.timeSignatures.erase(editTSIndex);
					pushHistory("Remove time signature", prev, score);
				}
			}

			ImGui::EndPopup();
		}
	}

	void ScoreEditor::debugInfo()
	{
		if (ImGui::Begin(IMGUI_TITLE(ICON_FA_BUG, "debug"), NULL, ImGuiWindowFlags_Static))
		{
			ImGuiIO io = ImGui::GetIO();
			ImGui::Text("Canvas Size: (%f, %f)", canvas.getSize().x, canvas.getSize().y);
			ImGui::Text("Timeline Offset: %f", canvas.getOffset());
			ImGui::Text("Cursor pos: %f", canvas.tickToPosition(currentTick));
			ImGui::Text("Mouse Pos: (%f, %f)", io.MousePos.x, io.MousePos.y);
			ImGui::Text("Timeline Mouse Pos: (%f, %f)", mousePos.x, mousePos.y);
			ImGui::Text("Hover lane: %d", hoverLane);
			ImGui::Text("Hover tick: %d\n CurrentTick: %d", hoverTick, currentTick);
			ImGui::Text("Current Measure: %d", accumulateMeasures(currentTick, TICKS_PER_BEAT, score.timeSignatures));
			ImGui::Checkbox("Show render stats", &showRenderStats);

			ImGui::Text("Undo Count: %d, Next: %s\nRedo Count: %d, Next: %s",
				history.undoCount(), history.peekUndo().c_str(),
				history.redoCount(), history.peekRedo().c_str());

			const char* noteHovering = isHoveringNote ? "yes" : "no";
			const char* noteHolding = isHoldingNote ? "yes" : "no";
			ImGui::Text("Hovering note: %s\nHolding note: %s", noteHovering, noteHolding);
			ImGui::Text("Selected count: %d", selection.count());

			const char* hasEase = selection.hasEase() ? "yes" : "no";
			const char* hasStep = selection.hasStep() ? "yes" : "no";
			ImGui::Text("Selection has ease: %s\nSelection has step: %s", hasEase, hasStep);
			ImGui::Text("Notes in clipboard: %d", Clipboard::count());

			auto it = score.notes.find(hoveringNote);
			ImGui::Text("Hover note: %s", (it == score.notes.end() ? "None" : ""));
			if (it != score.notes.end())
			{
				const Note& note = it->second;
				ImGui::Text("ID: %d\nType: %d\nFlick: %d\nCritical: %s\nTick: %d\nLane: %d\nWidth: %d\n",
					note.ID, note.getType(), note.flick, (note.critical ? "yes" : "no"), note.tick, note.lane, note.width);
			}
		}
		ImGui::End();
	}

	void ScoreEditor::update(float frameTime, Renderer* renderer, CommandManager* commandManager)
	{
		toolboxWindow();
		controlsWindow();
		updateTimeline(frameTime, renderer, commandManager);
		propertiesWindow();
		
		if (presetManager.updateWindow(score, selection.getSelection()))
		{
			cancelPaste();
			insertingPreset = true;
		}
		
		updateNoteSE();
		
#ifdef _DEBUG
		debugInfo();
#endif
		timeLastFrame = time;
		if (playing)
		{
			time += frameTime;
			currentTick = accumulateTicks(time, TICKS_PER_BEAT, score.tempoChanges);
			
			if (scrollMode == ScrollMode::Page)
			{
				float cursorPos = canvas.tickToPosition(currentTick);
				if (cursorPos > canvas.getOffset())
				{
					canvas.scrollPage(cursorPos);
				}
			}
			else if (scrollMode == ScrollMode::FollowCursor)
			{
				canvas.centerCursor(currentTick, playing, 1);
			}
		}
		else
		{
			time = accumulateDuration(currentTick, TICKS_PER_BEAT, score.tempoChanges);
		}
	}

	bool ScoreEditor::isWindowFocused() const
	{
		return windowFocused;
	}
}