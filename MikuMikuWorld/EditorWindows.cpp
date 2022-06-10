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
#include <algorithm>

namespace MikuMikuWorld
{
	void ScoreEditor::updateToolboxWindow()
	{
		if (ImGui::Begin(toolboxWindow))
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
			UI::addIntProperty("Note Width", defaultNoteWidth, 1, 12);
			UI::addSelectProperty("Step Type", defaultStepType, stepTypes, 3);
			UI::endPropertyColumns();
		}

		ImGui::End();
	}
	
	void ScoreEditor::updateTimeline(Renderer* renderer)
	{
		if (ImGui::Begin(timelineWindow, NULL, ImGuiWindowFlags_Static | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
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
			if (mouseInTimeline && !UI::isAnyPopupOpen())
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

			if (mouseInTimeline && (isPasting() || insertingPreset))
			{
				if (ImGui::IsMouseClicked(0))
					confirmPaste();
				else if (ImGui::IsMouseClicked(1))
					cancelPaste();
			}

			hasSelection = selectedNotes.size();
			hasSelectionEase = selectionHasEase();
			hasSelectionStep = selectionHasHoldStep();

			drawList->PopClipRect();
		}
		ImGui::End();
	}

	void ScoreEditor::contextMenu()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));
		if (ImGui::BeginPopupContextWindow(timelineWindow))
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

			if (ImGui::MenuItem(ICON_FA_GRIP_LINES_VERTICAL "\tFlip", "Ctrl + F", false, hasSelection))
				flipSelected();

			ImGui::Separator();
			if (ImGui::BeginMenu("Ease Type", hasSelectionEase))
			{
				if (ImGui::MenuItem("Linear"))
					setEase(EaseType::None);

				if (ImGui::MenuItem("Ease In"))
					setEase(EaseType::EaseIn);

				if (ImGui::MenuItem("Ease Out"))
					setEase(EaseType::EaseOut);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Step Type", hasSelectionStep))
			{
				if (ImGui::MenuItem("Visible"))
					setStepType(HoldStepType::Visible);

				if (ImGui::MenuItem("Invisible"))
					setStepType(HoldStepType::Invisible);

				if (ImGui::MenuItem("Ignored"))
					setStepType(HoldStepType::Ignored);

				ImGui::EndMenu();
			}

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
				UI::beginPropertyColumns();
				UI::addStringProperty("Title", workingData.title);
				UI::addStringProperty("Designer", workingData.designer);
				UI::addStringProperty("Artist", workingData.artist);
				UI::endPropertyColumns();
			}

			if (ImGui::CollapsingHeader(ICON_FA_VOLUME_UP " Audio", ImGuiTreeNodeFlags_DefaultOpen))
			{
				UI::beginPropertyColumns();

				std::string filename = musicFile;
				int filePickResult = UI::addFileProperty("Music File", filename);
				if (filePickResult == 1 && filename != musicFile)
				{
					loadMusic(filename);
				}
				else if (filePickResult == 2)
				{
					if (FileDialog::openFile(filename, FileType::AudioFile) && filename != musicFile)
						loadMusic(filename);
				}

				float offset = musicOffset;
				UI::addFloatProperty("Music Offset", offset, "%.03fms");
				if (offset != musicOffset)
				{
					musicOffset = offset;
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
				std::vector<int> counts = countNotes(score);
				int taps = counts[0], flicks = counts[1], holds = counts[2], steps = counts[3], combo = counts[4];
				int total = taps + flicks + holds + steps;

				UI::beginPropertyColumns();
				UI::addReadOnlyProperty("Taps", std::to_string(taps));
				UI::addReadOnlyProperty("Flicks", std::to_string(flicks));
				UI::addReadOnlyProperty("Holds", std::to_string(holds));
				UI::addReadOnlyProperty("Steps", std::to_string(steps));
				UI::addReadOnlyProperty("Total", std::to_string(total));
				UI::addReadOnlyProperty("Combo", std::to_string(combo));
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

	void ScoreEditor::updateControls()
	{
		if (ImGui::Begin(controlsWindow))
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
			float _zoom = zoom;
			if (UI::transparentButton(ICON_FA_SEARCH_MINUS, UI::btnSmall))
				_zoom -= 0.25f;

			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - (ImGui::GetStyle().ItemSpacing.x));
			ImGui::SliderFloat("##zoom", &_zoom, MIN_ZOOM, MAX_ZOOM, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
			ImGui::SameLine();

			if (UI::transparentButton(ICON_FA_SEARCH_PLUS, UI::btnSmall))
				_zoom += 0.25f;

			if (zoom != _zoom)
				setZoom(_zoom);

			UI::endPropertyColumns();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
			ImGui::Checkbox("Show Step Outlines", &drawHoldStepOutline);
		}

		ImGui::End();
	}

	void ScoreEditor::updatePresetsWindow()
	{
		if (ImGui::Begin(presetsWindow))
		{
			static std::string presetName = "";
			static std::string presetDesc = "";
			int removePattern = -1;

			presetFilter.Draw("##preset_filter", ICON_FA_SEARCH " Search...", -1);

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.15f, 1.00f));
			float windowHeight = ImGui::GetContentRegionAvail().y - ((ImGui::GetFrameHeight() * 3.0f) + 50);
			if (ImGui::BeginChild("presets_child_window", ImVec2(-1, windowHeight), true))
			{
				const auto& presets = presetManager.getPresets();

				if (!presets.size())
				{
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
					Utilities::ImGuiCenteredText("No presets available.");
					ImGui::PopStyleVar();
				}
				else
				{
					for (const auto& [id, preset] : presets)
					{
						if (!presetFilter.PassFilter(preset.getName().c_str()))
							continue;

						std::string strID = std::to_string(id) + "_" + preset.getName();
						ImGui::PushID(strID.c_str());
						if (ImGui::Button(preset.getName().c_str(), ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 2.0f, UI::btnSmall.y + 2.0f)))
						{
							if (isPasting())
								cancelPaste();

							insertingPreset = true;
							presetNotes = preset.notes;
							presetHolds = preset.holds;
						}

						if (preset.description.size())
							UI::tooltip(preset.description.c_str());

						ImGui::SameLine();
						if (UI::transparentButton(ICON_FA_TRASH, ImVec2(UI::btnSmall.x, UI::btnSmall.y + 2.0f)))
							removePattern = id;

						ImGui::PopID();
					}
				}
			}
			ImGui::EndChild();

			UI::beginPropertyColumns();
			UI::addStringProperty("Name", presetName);
			UI::addMultilineString("Description", presetDesc);
			UI::endPropertyColumns();
			ImGui::Separator();

			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !presetName.size() || !selectedNotes.size());
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * (!presetName.size() || !selectedNotes.size())));
			if (ImGui::Button("Create Preset", ImVec2(-1, UI::btnSmall.y + 2.0f)))
			{
				presetManager.createPreset(score, selectedNotes, presetName, presetDesc);
				presetName.clear();
				presetDesc.clear();
			}

			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			ImGui::PopItemFlag();

			if (removePattern != -1)
				presetManager.removePreset(removePattern);
		}

		ImGui::End();
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
		updateToolboxWindow();
		updateControls();
		updateTimeline(renderer);
		updateScoreDetails();
		updatePresetsWindow();
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
				float cursorPos = tickToPosition(currentTick);
				if (cursorPos > timelineOffset)
					timelineOffset = cursorPos + canvasSize.y;
			}
			else if (scrollMode == ScrollMode::Smooth)
			{
				centerCursor(1);
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