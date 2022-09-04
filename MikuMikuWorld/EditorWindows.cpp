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
#include <algorithm>

namespace MikuMikuWorld
{
	constexpr const char* uiFlickTypes[] =
	{
		"None", "Up", "Left", "Right"
	};

	constexpr const char* uiStepTypes[] =
	{
		"Visible", "Invisible", "Ignored"
	};

	constexpr const char* uiEaseTypes[] =
	{
		"Linear", "Ease-In", "Ease-Out"
	};

	void ScoreEditor::toolboxWindow()
	{
		if (ImGui::Begin(toolboxWindowTitle))
		{
			ImVec2 btnSz{ ImGui::GetContentRegionAvail().x, 35.0f };
			for (int i = 0; i < (int)TimelineMode::TimelineToolMax; ++i)
			{
				ImGui::PushID(i);
				bool highlight = (int)currentMode == i;
				if (highlight)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Header]);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_Header]);
				}

				if (ImGui::Button(timelineModes[i], btnSz))
					changeMode((TimelineMode)i);

				std::string txt = "(" + std::to_string(i + 1) + ") " + timelineModes[i];
				UI::tooltip(txt.c_str());

				if (highlight)
					ImGui::PopStyleColor(2);

				ImGui::PopID();
			}

			ImGui::Separator();
			UI::beginPropertyColumns();

			if (currentMode == TimelineMode::InsertBPM)
			{
				UI::addFloatProperty("BPM", defaultBPM, "%g BPM");
				defaultBPM = std::clamp(defaultBPM, MIN_BPM, MAX_BPM);
			}
			else if (currentMode == TimelineMode::InsertTimeSign)
			{
				UI::addFractionProperty("Time Signature", defaultTimeSignN, defaultTimeSignD);
				defaultTimeSignN = std::clamp(defaultTimeSignN, MIN_TIME_SIGN, MAX_TIME_SIGN);
				defaultTimeSignD = std::clamp(defaultTimeSignD, MIN_TIME_SIGN, MAX_TIME_SIGN);
			}
			else
			{
				UI::addIntProperty("Note Width", defaultNoteWidth, 1, 12);
				UI::addSelectProperty("Step Type", defaultStepType, uiStepTypes, TXT_ARR_SZ(uiStepTypes));
			}

			UI::endPropertyColumns();
		}

		ImGui::End();
	}
	
	void ScoreEditor::updateTimeline(float frameTime, Renderer* renderer)
	{
		if (ImGui::Begin(timelineWindowTitle, NULL, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			windowFocused = ImGui::IsWindowFocused();
			if (canvas.isMouseInCanvas() && !UI::isAnyPopupOpen())
			{
				// get mouse position relative to canvas
				mousePos = ImGui::GetIO().MousePos - canvas.getPosition();
				mousePos.y -= canvas.getOffset();

				if (!isHoveringNote && !isHoldingNote && !insertingHold)
				{
					if (ImGui::IsMouseDown(0) && ImGui::IsMouseDragPastThreshold(0, 10.0f))
						dragging = currentMode == TimelineMode::Select;

					if (ImGui::IsMouseClicked(0))
					{
						if (!InputListener::isCtrlDown() && !InputListener::isAltDown() && !ImGui::IsPopupOpen(timelineWindowTitle))
							selection.clear();

						if (currentMode == TimelineMode::Select)
							dragStart = mousePos;
					}
				}
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->PushClipRect(canvas.getBoundaries().Min, canvas.getBoundaries().Max, true);

			canvas.update(frameTime);
			canvas.updateScorllingPosition(frameTime);
			canvas.drawBackground(renderer);
			canvas.drawLanesBackground();
			
			drawMeasures();
			updateTempoChanges();
			updateTimeSignatures();
			
			for (const auto& skill : score.skills)
				drawSkill(skill);

			drawFever(score.fever);
			drawLanes();

			contextMenu();
			updateCursor();

			noteCtrlHeight = canvas.getNotesHeight();
			isHoveringNote = false;
			hoveringNote = -1;
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

			selection.update(score);
			drawList->PopClipRect();
		}
		ImGui::End();
	}

	void ScoreEditor::contextMenu()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 10));
		if (ImGui::BeginPopupContextWindow(timelineWindowTitle))
		{
			if (ImGui::MenuItem(ICON_FA_TRASH "\tDelete", "Delete", false, selection.hasSelection()))
				deleteSelected();

			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_COPY "\tCopy", "Ctrl + C", false, selection.hasSelection()))
				copy();

			if (ImGui::MenuItem(ICON_FA_PASTE "\tPaste", "Ctrl + V", false, Clipboard::hasData()))
				paste();

			if (ImGui::MenuItem(ICON_FA_PASTE "\tFlip Paste", "Ctrl + Shift + V", false, Clipboard::hasData()))
				flipPaste();

			if (ImGui::MenuItem(ICON_FA_GRIP_LINES_VERTICAL "\tFlip", "Ctrl + F", false, selection.hasSelection()))
				flipSelected();

			ImGui::Separator();
			if (ImGui::BeginMenu("Ease Type", selection.hasEase()))
			{
				for (int i = 0; i < TXT_ARR_SZ(uiEaseTypes); ++i)
				{
					if (ImGui::MenuItem(uiEaseTypes[i]))
						setEase((EaseType)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Step Type", selection.hasStep()))
			{
				for (int i = 0; i < TXT_ARR_SZ(uiStepTypes); ++i)
				{
					if (ImGui::MenuItem(uiStepTypes[i]))
						setStepType((HoldStepType)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Flick Type", selection.hasFlickable()))
			{
				for (int i = 0; i < TXT_ARR_SZ(uiFlickTypes); ++i)
				{
					if (ImGui::MenuItem(uiFlickTypes[i]))
						setFlick((FlickType)i);
				}
				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

	void ScoreEditor::propertiesWindow()
	{
		if (ImGui::Begin(detailsWindowTitle, NULL, ImGuiWindowFlags_Static))
		{
			if (ImGui::CollapsingHeader(ICON_FA_ALIGN_LEFT " Metadata", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				UI::addStringProperty("Title", workingData.title);
				UI::addStringProperty("Designer", workingData.designer);
				UI::addStringProperty("Artist", workingData.artist);

				int result = UI::addFileProperty("Jacket", workingData.jacket.filename);
				if (result == 2)
				{
					std::string name;
					if (FileDialog::openFile(name, FileType::ImageFile))
					{
						ResourceManager::loadTexture(name);
						int texIndex = ResourceManager::getTexture(File::getFilenameWithoutExtension(name));
						workingData.jacket.texID = ResourceManager::textures[texIndex].getID();
						workingData.jacket.filename = name;

						canvas.changeBackground(ResourceManager::textures[texIndex]);
					}
				}
				UI::endPropertyColumns();

				// draw jacket
				ImDrawList* drawList = ImGui::GetWindowDrawList();

				// center jacket
				float windowWidth = ImGui::GetWindowSize().x;
				float jacketSize = std::min(200.0f, windowWidth * 0.9f);
				float xOffset = (windowWidth - 10.0f - jacketSize) / 2.0f;

				ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, 20.0f));
				ImVec2 cursorPos1 = ImGui::GetCursorScreenPos() + ImVec2(xOffset, 0);
				ImVec2 cursorPos2 = ImGui::GetCursorScreenPos() + ImVec2(xOffset + 8.0f, 8.0f);
				drawList->AddRectFilled(cursorPos1, cursorPos1 + ImVec2(jacketSize, jacketSize), 0xff151515);
				if (workingData.jacket.texID != -1)
				{
					drawList->AddImage((void*)workingData.jacket.texID, cursorPos2, cursorPos2 + ImVec2(jacketSize, jacketSize));
				}
				ImGui::SetCursorScreenPos(ImGui::GetCursorScreenPos() + ImVec2(0, jacketSize + 20.0f));
			}

			if (ImGui::CollapsingHeader(ICON_FA_VOLUME_UP " Audio", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				std::string filename = workingData.musicFilename;
				int filePickResult = UI::addFileProperty("Music File", filename);
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
				UI::addFloatProperty("Music Offset", offset, "%.03fms");
				if (offset != workingData.musicOffset)
				{
					workingData.musicOffset = offset;
					audio.setBGMOffset(time, offset);
				}

				// volume controls
				float master = masterVolume, bgm = bgmVolume, se = seVolume;
				UI::addPercentSliderProperty("Master Volume", master);
				UI::addPercentSliderProperty("BGM Volume", bgm);
				UI::addPercentSliderProperty("SE Volume", se);

				if (master != masterVolume)
				{
					audio.setMasterVolume(master);
					masterVolume = master;
				}

				if (bgm != bgmVolume)
				{
					audio.setBGMVolume(bgm);
					bgmVolume = bgm;
				}

				if (se != seVolume)
				{
					audio.setSEVolume(se);
					seVolume = se;
				}

				UI::endPropertyColumns();
			}

			if (ImGui::CollapsingHeader(ICON_FA_CHART_BAR " Statistics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();
				UI::addReadOnlyProperty("Taps", std::to_string(stats.getTaps()));
				UI::addReadOnlyProperty("Flicks", std::to_string(stats.getFlicks()));
				UI::addReadOnlyProperty("Holds", std::to_string(stats.getHolds()));
				UI::addReadOnlyProperty("Steps", std::to_string(stats.getSteps()));
				UI::addReadOnlyProperty("Total", std::to_string(stats.getTotal()));
				UI::addReadOnlyProperty("Combo", std::to_string(stats.getCombo()));
				UI::endPropertyColumns();
			}
		}

		ImGui::End();
	}

	std::string ScoreEditor::getDivisonString(int divIndex)
	{
		int count = sizeof(divisions) / sizeof(int);
		bool customSelected = divIndex == count - 1;

		std::string prefix = "1/";
		return (customSelected ? "Custom " : "") +
			prefix + std::to_string((customSelected ? customDivision : divisions[divIndex]));
	}

	void ScoreEditor::controlsWindow()
	{
		if (ImGui::Begin(controlsWindowTitle))
		{
			int measure = accumulateMeasures(currentTick, TICKS_PER_BEAT, score.timeSignatures);
			const TimeSignature& ts = score.timeSignatures[findTimeSignature(measure, score.timeSignatures)];
			const Tempo& tempo = getTempoAt(currentTick, score.tempoChanges);

			std::string timeStr = formatString("Time: %02d:%02d:%02d", (int)time / 60, (int)time % 60, (int)((time - (int)time) * 100));
			std::string signStr = formatString("\t|\t%d/%d, ", ts.numerator, ts.denominator);
			std::string tempoStr = formatString("%g BPM", tempo.bpm);

			Utilities::ImGuiCenteredText(timeStr + signStr + tempoStr);

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

			static int m = 0;
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			
			UI::beginPropertyColumns();
			UI::propertyLabel("Goto Measure");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
			ImGui::InputInt("##goto_measure", &m, 0, 0);

			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_ARROW_RIGHT, UI::btnSmall))
				gotoMeasure(m);

			ImGui::NextColumn();
			UI::propertyLabel("Divison");

			int divCount = sizeof(divisions) / sizeof(int);
			if (ImGui::BeginCombo("##division", getDivisonString(selectedDivision).c_str()))
			{
				for (int i = 0; i < divCount; ++i)
				{
					const bool custom = i == divCount - 1;
					const bool selected = selectedDivision == i;
					if (ImGui::Selectable(getDivisonString(i).c_str(), selected))
					{
						selectedDivision = i;
						if (selectedDivision < divCount - 1)
							division = divisions[selectedDivision];
						else
							division = customDivision;
					}
				}
				
				ImGui::EndCombo();
			}

			ImGui::NextColumn();
			UI::propertyLabel("Custom Divison");
			if (ImGui::InputInt("##custom_div", &customDivision, 1, 4))
			{
				customDivision = std::clamp(customDivision, 4, 1920);
				if ((selectedDivision == divCount - 1))
					division = customDivision;
			}

			ImGui::NextColumn();
			UI::addSelectProperty("Scroll Mode", scrollMode, scrollModes, (int)ScrollMode::ScrollModeMax);

			UI::propertyLabel("Zoom");
			float _zoom = canvas.getZoom();
			if (UI::transparentButton(ICON_FA_SEARCH_MINUS, UI::btnSmall))
				_zoom -= 0.25f;

			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - (ImGui::GetStyle().ItemSpacing.x));
			ImGui::SliderFloat("##zoom", &_zoom, MIN_ZOOM, MAX_ZOOM, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SameLine();

			if (UI::transparentButton(ICON_FA_SEARCH_PLUS, UI::btnSmall))
				_zoom += 0.25f;

			if (canvas.getZoom() != _zoom)
				canvas.setZoom(_zoom);

			UI::endPropertyColumns();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Checkbox("Show Step Outlines", &drawHoldStepOutline);
		}

		ImGui::End();
	}

	void ScoreEditor::debugInfo()
	{
		if (ImGui::Begin(Title, NULL, ImGuiWindowFlags_Static))
		{
			ImGuiIO io = ImGui::GetIO();
			ImGui::Text("Canvas Size: (%f, %f)", canvas.getSize().x, canvas.getSize().y);
			ImGui::Text("Timeline Offset: %f", canvas.getOffset());
			ImGui::Text("Cursor pos: %f", canvas.tickToPosition(currentTick));
			ImGui::Text("Mouse Pos: (%f, %f)", io.MousePos.x, io.MousePos.y);
			ImGui::Text("Timeline Mouse Pos: (%f, %f)", mousePos.x, mousePos.y);
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

	void ScoreEditor::update(float frameTime, Renderer* renderer)
	{
		toolboxWindow();
		controlsWindow();
		updateTimeline(frameTime, renderer);
		propertiesWindow();
		
		if (presetManager.updateWindow(score, selection.getSelection()))
		{
			if (isPasting())
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
			else if (scrollMode == ScrollMode::Smooth)
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