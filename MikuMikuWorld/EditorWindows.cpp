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
#include <algorithm>

namespace MikuMikuWorld
{
	void ScoreEditor::updateTimeline(Renderer* renderer)
	{
		ImGui::Begin(timelineWindow, NULL, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::BeginColumns("timeline_cols", 2, ImGuiOldColumnFlags_NoResize);
		ImGui::SetColumnWidth(0, 180.0f);

		ImVec2 btnSz{ ImGui::GetContentRegionAvail().x, 50.0f };

		const Texture& tex = ResourceManager::textures[0];
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

			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltipEx(0, ImGuiTooltipFlags_OverridePreviousTooltip);
				ImGui::Text("%s (%d)", timelineModes[i], i + 1);
				ImGui::EndTooltip();
			}

			if (highlight)
				ImGui::PopStyleColor(2);

			ImGui::PopID();
		}

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Text("Mode: %s", timelineModes[(int)currentMode]);
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

		ImGui::Text("Default Note Width");
		ImGui::SetNextItemWidth(-1);
		if (ImGui::InputInt("##default_note_width", &defaultNoteWidth, 1, 2))
			defaultNoteWidth = std::clamp(defaultNoteWidth, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH);

		static int m = 0;
		ImGui::Text("Goto Measure");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
		ImGui::InputInt("##goto_measure", &m, 0, 0);
		
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_ARROW_RIGHT, btnSmall))
			gotoMeasure(m);

		ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
		ImGui::Text("Time: %02d:%02d:%02d", (int)time / 60, (int)time % 60, (int)((time - (int)time) * 100));

		if (transparentButton(ICON_FA_STOP))
			stop();

		ImGui::SameLine();
		if (transparentButton(ICON_FA_BACKWARD, btnNormal, true))
			previousTick();

		ImGui::SameLine();
		if (transparentButton(playing ? ICON_FA_PAUSE : ICON_FA_PLAY))
			togglePlaying();

		ImGui::SameLine();
		if (transparentButton(ICON_FA_FORWARD, btnNormal, true))
			nextTick();

		ImGui::SameLine();
		if (transparentButton(ICON_FA_REDO_ALT))
			restart();

		ImGui::NextColumn();
		updateControls();

		canvasSize = ImGui::GetContentRegionAvail();
		canvasPos = ImGui::GetCursorScreenPos();
		boundaries = ImRect(canvasPos, canvasPos + canvasSize);
		mouseInTimeline = ImGui::IsMouseHoveringRect(canvasPos, canvasPos + canvasSize);
		windowFocused = ImGui::IsWindowFocused();

		timelineWidth = NUM_LANES * laneWidth;
		noteCtrlHeight = notesHeight;
		laneOffset = (canvasSize.x * 0.5f) - (timelineWidth * 0.5f);

		ImGuiIO& io = ImGui::GetIO();
		timelineMinOffset = ImGui::GetWindowHeight() - 200.0f;

		// mouse input
		if (mouseInTimeline && !isAnyPopupOpen())
		{
			float mouseWheelDelta = io.MouseWheel;
			if (InputListener::isCtrlDown())
				setZoom(zoom + (mouseWheelDelta * 0.1f));
			else
				timelineOffset += mouseWheelDelta * (InputListener::isShiftDown() ? 200.0f : 50.0f);

			mousePos = io.MousePos - canvasPos;
			mousePos.y -= timelineOffset;

			if (!isHoveringNote && !isHoldingNote && !insertingHold && ImGui::IsWindowFocused())
			{
				if (ImGui::IsMouseDown(0) && ImGui::IsMouseDragPastThreshold(0, 10.0f))
					dragging = true;

				if (ImGui::IsMouseClicked(0))
				{
					if (!InputListener::isCtrlDown() && !InputListener::isAltDown() && !ImGui::IsPopupOpen(timelineWindow))
						selectedNotes.clear();

					if (currentMode == TimelineMode::Select)
						dragStart = mousePos;
				}
			}
		}

		timelineOffset = std::max(timelineOffset, timelineMinOffset);

		ImGui::ItemSize(boundaries);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(boundaries.Min, boundaries.Max, true);

		effectiveTickHeight = TICK_HEIGHT * zoom;
		isHoveringNote = false;
		hoveringNote = -1;

		contextMenu();
		drawMeasures();
		updateTempoChanges();
		updateTimeSignatures();
		drawLanes();
		updateCursor();
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

		if (isPasting())
		{
			if (ImGui::IsMouseClicked(0))
				confirmPaste();
		}

		hasSelection = selectedNotes.size();
		hasSelectionEase = selectionHasEase();
		hasSelectionStep = selectionHasHoldStep();

		drawList->PopClipRect();

		ImGui::EndColumns();
		ImGui::End();
	}

	void ScoreEditor::contextMenu()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
		if (ImGui::BeginPopupContextWindow("Notes Timeline"))
		{
			if (ImGui::MenuItem(ICON_FA_TRASH "\tDelete", "Delete", false, hasSelection))
				deleteSelected();

			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_COPY "\tCopy", "Ctrl + C", false, hasSelection))
				copy();

			if (ImGui::MenuItem(ICON_FA_PASTE "\tPaste", "Ctrl + V", false, hasClipboard()))
				paste();

			if (ImGui::MenuItem(ICON_FA_PASTE "\tFlip Paste", "Ctrl + Shift + V", false, hasClipboard()))
				flipPaste();

			if (ImGui::MenuItem(ICON_FA_GRIP_LINES_VERTICAL "\tFlip Horizontally", "Ctrl + F", false, hasSelection))
				flipSelected();

			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_SLASH "\tLinear", NULL, false, hasSelectionEase))
				setEase(EaseType::None);

			if (ImGui::MenuItem(ICON_FA_BEZIER_CURVE "\tEase In", NULL, false, hasSelectionEase))
				setEase(EaseType::EaseIn);

			if (ImGui::MenuItem(ICON_FA_BEZIER_CURVE "\tEase Out", NULL, false, hasSelectionEase))
				setEase(EaseType::EaseOut);

			ImGui::Separator();
			if (ImGui::MenuItem(ICON_FA_EYE "\tVisible", NULL, false, hasSelectionStep))
				setStepVisibility(HoldStepType::Visible);

			if (ImGui::MenuItem(ICON_FA_EYE_SLASH "\tInvisible", NULL, false, hasSelectionStep))
				setStepVisibility(HoldStepType::Invisible);

			if (ImGui::MenuItem(ICON_FA_EYE "\tIgnored", NULL, false, hasSelectionStep))
				setStepVisibility(HoldStepType::Ignored);

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

	void ScoreEditor::updateScoreDetails()
	{
		if (ImGui::Begin(detailsWindow, NULL, ImGuiWindowFlags_Static))
		{
			if (ImGui::CollapsingHeader(ICON_FA_ALIGN_LEFT " Metadata", ImGuiTreeNodeFlags_DefaultOpen))
			{
				beginPropertyColumns();

				addStringProperty("Title", workingData.title);
				addStringProperty("Designer", workingData.designer);
				addStringProperty("Artist", workingData.artist);
				addIntProperty("Difficulty", workingData.difficulty, 1, 40);
				
				endPropertyColumns();
			}

			if (ImGui::CollapsingHeader(ICON_FA_VOLUME_UP " Audio", ImGuiTreeNodeFlags_DefaultOpen))
			{
				beginPropertyColumns();

				if (addFileProperty("Music File", musicFile))
				{
					std::string filename;
					if (FileDialog::openFile(filename, FileType::AudioFile))
					{
						musicFile = filename;
						audio.changeBGM(filename);
					}
				}

				float offset = musicOffset;
				addFloatProperty("Music Offset", offset, "%.03fms");
				if (offset != musicOffset)
				{
					musicOffset = offset;
					audio.setBGMOffset(time, offset);
				}

				// volume controls
				float master = masterVolume, bgm = bgmVolume, se = seVolume;
				addPercentSliderProperty("Master Volume", master);
				addPercentSliderProperty("BGM Volume", bgm);
				addPercentSliderProperty("SE Volume", se);

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

				endPropertyColumns();
			}

			if (ImGui::CollapsingHeader(ICON_FA_CHART_BAR " Statistics", ImGuiTreeNodeFlags_DefaultOpen))
			{
				std::vector<int> counts = countNotes(score);
				int taps = counts[0], flicks = counts[1], holds = counts[2], steps = counts[3], combo = counts[4];
				int total = taps + flicks + holds + steps;

				beginPropertyColumns();
				addReadOnlyProperty("Taps", std::to_string(taps));
				addReadOnlyProperty("Flicks", std::to_string(flicks));
				addReadOnlyProperty("Holds", std::to_string(holds));
				addReadOnlyProperty("Steps", std::to_string(steps));
				addReadOnlyProperty("Total", std::to_string(total));
				addReadOnlyProperty("Combo", std::to_string(combo));
				endPropertyColumns();
			}
		}

		ImGui::End();
	}

	void ScoreEditor::updateControls()
	{
		ImGui::Text("Division");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(100);
		std::string divPrefix = "1/";
		std::string preview = divPrefix + std::to_string(divisions[selectedDivision]);
		if (ImGui::BeginCombo("##division", preview.c_str()))
		{
			for (int i = 0; i < sizeof(divisions) / sizeof(int); ++i)
			{
				const bool selected = selectedDivision == i;
				std::string div = divPrefix + std::to_string(divisions[i]);
				if (ImGui::Selectable(div.c_str(), selected))
				{
					selectedDivision = i;
					division = divisions[selectedDivision];
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, ImGui::GetStyle().ItemSpacing.y));
		
		ImGui::SameLine();
		ImGui::Text("Zoom");
		ImGui::SameLine();
		float _zoom = zoom;
		if (transparentButton(ICON_FA_SEARCH_MINUS, btnSmall))
			_zoom -= 0.25f;

		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::SliderFloat("##zoom", &_zoom, MIN_ZOOM, MAX_ZOOM, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SameLine();

		if (transparentButton(ICON_FA_SEARCH_PLUS, btnSmall))
			_zoom += 0.25f;

		if (zoom != _zoom)
			setZoom(_zoom);

		ImGui::SameLine();
		ImGui::Checkbox("Show Step Outlines", &drawHoldStepOutline);

		ImGui::PopStyleVar();
	}

	void ScoreEditor::debugInfo()
	{
		if (ImGui::Begin(debugWindow, NULL, ImGuiWindowFlags_Static))
		{
			ImGuiIO io = ImGui::GetIO();
			ImGui::Text("Canvas Size: (%f, %f)", canvasSize.x, canvasSize.y);
			ImGui::Text("Timeline Offset: %f", timelineOffset);
			ImGui::Text("Cursor pos: %f", tickToPosition(currentTick));
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
			ImGui::Text("Selected count: %d", selectedNotes.size());

			const char* hasEase = hasSelectionEase ? "yes" : "no";
			const char* hasStep = hasSelectionStep ? "yes" : "no";
			ImGui::Text("Selection has ease: %s\nSelection has step: %s", hasEase, hasStep);
			ImGui::Text("Notes in clipboard: %d", copyNotes.size());

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
		updateTimeline(renderer);
		updateScoreDetails();
		updateControls();
		updateNoteSE();
		
#ifdef _DEBUG
		debugInfo();
#endif
		timeLastFrame = time;
		if (playing)
		{
			time += frameTime;
			currentTick = accumulateTicks(time, TICKS_PER_BEAT, score.tempoChanges);
			centerCursor(1);
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