#include "ScoreEditorTimeline.h"
#include "Constants.h"
#include "UI.h"
#include "Colors.h"
#include "ResourceManager.h"
#include "Tempo.h"
#include "Utilities.h"
#include "NoteGraphics.h"
#include "ApplicationConfiguration.h"
#include "Application.h"
#include <algorithm>

namespace MikuMikuWorld
{
	bool eventControl(float xPos, Vector2 pos, ImU32 color, const char* txt, bool enabled)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return false;

		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);

		ImGui::PushID(pos.y);
		bool activated = UI::coloredButton(txt, { pos.x, pos.y - ImGui::GetFrameHeightWithSpacing() }, { -1, -1 }, color, enabled);
		ImGui::PopID();
		drawList->AddLine({ xPos, pos.y }, { pos.x + ImGui::GetItemRectSize().x, pos.y }, color, primaryLineThickness);

		return activated;
	}

	float ScoreEditorTimeline::getNoteYPosFromTick(int tick) const
	{
		return position.y + tickToPosition(tick) - visualOffset + size.y;
	}

	int ScoreEditorTimeline::positionToTick(float pos) const
	{
		return roundf(pos / (unitHeight * zoom));
	}

	float ScoreEditorTimeline::tickToPosition(int tick) const
	{
		return tick * unitHeight * zoom;
	}

	int ScoreEditorTimeline::positionToLane(float pos) const
	{
		return floor((pos - laneOffset) / laneWidth);
	}

	float ScoreEditorTimeline::laneToPosition(float lane) const
	{
		return laneOffset + (lane * laneWidth);
	}

	bool ScoreEditorTimeline::isNoteVisible(const Note& note, int offsetTicks) const
	{
		const float y = getNoteYPosFromTick(note.tick + offsetTicks);
		return y >= 0 && y <= size.y + position.y + 100;
	}

	void ScoreEditorTimeline::setZoom(float value)
	{
		int tick = positionToTick(offset - size.y);
		float x1 = position.y - tickToPosition(tick) + offset;

		zoom = std::clamp(value, MIN_ZOOM, MAX_ZOOM);
		config.zoom = zoom;

		float x2 = position.y - tickToPosition(tick) + offset;
		visualOffset = offset = std::max(offset + x1 - x2, minOffset); // prevent jittery movement when zooming
	}

	int ScoreEditorTimeline::snapTickFromPos(float posY, const ScoreContext& context)
	{
		return snapTick(positionToTick(posY), division, context.score.timeSignatures);
	}

	int ScoreEditorTimeline::laneFromCenterPosition(int lane, int width)
	{
		return std::clamp(lane - (width / 2), MIN_LANE, MAX_LANE - width + 1);
	}

	void ScoreEditorTimeline::focusCursor(ScoreContext& context, Direction direction)
	{
		float cursorY = tickToPosition(context.currentTick);
		if (direction == Direction::Down)
		{
			float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
			if (cursorY <= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
		else
		{
			float timelineOffset = size.y * config.cursorPositionThreshold;
			if (cursorY >= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
	}

	void ScoreEditorTimeline::updateScrollbar()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
		float paddingY = 30.0f;
		ImVec2 windowEndTop = ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - scrollbarWidth - 4, paddingY };
		ImVec2 windowEndBottom = windowEndTop + ImVec2{ scrollbarWidth + 2, ImGui::GetWindowSize().y - (paddingY * 1.3f) - UI::toolbarBtnSize.y - 5 };

		// calculate handle height
		float heightRatio = size.y / ((maxOffset - minOffset) * zoom);
		float handleHeight = std::max(20.0f, ((windowEndBottom.y - windowEndTop.y) * heightRatio) + 30.0f);
		float scrollHeight = windowEndBottom.y - windowEndTop.y - handleHeight;

		// calculate handle position
		float currentOffset = offset - minOffset;
		float positionRatio = std::min(1.0f, currentOffset / ((maxOffset * zoom) - minOffset));
		float handlePosition = windowEndBottom.y - (scrollHeight * positionRatio) - handleHeight;

		// make handle slightly narrower than the background
		ImVec2 scrollHandleMin = ImVec2{ windowEndTop.x + 2, handlePosition };
		ImVec2 scrollHandleMax = ImVec2{ windowEndTop.x + scrollbarWidth - 2, handlePosition + handleHeight };

		// handle "button"
		ImGuiCol handleColor = ImGuiCol_ScrollbarGrab;
		ImGui::SetCursorScreenPos(scrollHandleMin);
		ImGui::InvisibleButton("##scroll_handle", ImVec2{ scrollbarWidth, handleHeight });
		if (ImGui::IsItemHovered())
			handleColor = ImGuiCol_ScrollbarGrabHovered;

		if (ImGui::IsItemActivated())
			scrollStartY = ImGui::GetMousePos().y;

		if (ImGui::IsItemActive())
		{
			handleColor = ImGuiCol_ScrollbarGrabActive;
			float dragDeltaY = scrollStartY - ImGui::GetMousePos().y;
			if (abs(dragDeltaY) > 0)
			{
				// convert handle position to timeline offset
				handlePosition -= dragDeltaY;
				positionRatio = std::min(1.0f, 1 - ((handlePosition - windowEndTop.y) / scrollHeight));
				float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

				offset = newOffset + minOffset;
				scrollStartY = ImGui::GetMousePos().y;
			}
		}

		if (!ImGui::IsItemActive() && positionRatio >= 0.92f)
			maxOffset += 2000;

		ImGui::SetCursorScreenPos(windowEndTop);
		ImGui::InvisibleButton("##scroll_background", ImVec2{ scrollbarWidth, scrollHeight + handleHeight }, ImGuiButtonFlags_AllowItemOverlap);
		if (ImGui::IsItemActivated())
		{
			float yPos = std::clamp(ImGui::GetMousePos().y, windowEndTop.y, windowEndBottom.y - handleHeight);

			// convert handle position to timeline offset
			positionRatio = std::clamp(1 - ((yPos - windowEndTop.y)) / scrollHeight, 0.0f, 1.0f);
			float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

			offset = newOffset + minOffset;
		}

		ImU32 scrollBgColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg));
		ImU32 scrollHandleColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(handleColor));
		drawList->AddRectFilled(windowEndTop, windowEndBottom, scrollBgColor, 0);
		drawList->AddRectFilled(scrollHandleMin, scrollHandleMax, scrollHandleColor, ImGui::GetStyle().ScrollbarRounding, ImDrawFlags_RoundCornersAll);
	}

	void ScoreEditorTimeline::calculateMaxOffsetFromScore(const Score& score)
	{
		int maxTick = 0;
		for (const auto& [id, note] : score.notes)
			maxTick = std::max(maxTick, note.tick);

		// current offset maybe greater than calculated offset from score
		maxOffset = std::max(offset, (maxTick * unitHeight) + minOffset + 1000);
	}

	void ScoreEditorTimeline::updateScrollingPosition()
	{
		if (config.useSmoothScrolling)
		{
			float scrollAmount = offset - visualOffset;
			float remainingScroll = abs(scrollAmount);
			float delta = scrollAmount / (config.smoothScrollingTime / (ImGui::GetIO().DeltaTime * 1000));

			visualOffset += std::min(remainingScroll, delta);
			remainingScroll = std::max(0.0f, remainingScroll - abs(delta));
		}
		else
		{
			visualOffset = offset;
		}
	}

	void ScoreEditorTimeline::contextMenu(ScoreContext& context)
	{
		if (ImGui::BeginPopupContextWindow(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
		{
			if (ImGui::MenuItem(getString("delete"), ToShortcutString(config.input.deleteSelection), false, context.selectedNotes.size()))
				context.deleteSelection();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("cut"), ToShortcutString(config.input.cutSelection), false, context.selectedNotes.size()))
				context.cutSelection();

			if (ImGui::MenuItem(getString("copy"), ToShortcutString(config.input.copySelection), false, context.selectedNotes.size()))
				context.copySelection();

			if (ImGui::MenuItem(getString("paste"), ToShortcutString(config.input.paste)))
				context.paste(false);

			if (ImGui::MenuItem(getString("flip_paste"), ToShortcutString(config.input.flipPaste)))
				context.paste(true);

			if (ImGui::MenuItem(getString("flip"), ToShortcutString(config.input.flip), false, context.selectedNotes.size()))
				context.flipSelection();

			const bool hasEase = context.selectionHasEase();
			const bool hasStep = context.selectionHasStep();
			const bool hasFlick = context.selectionHasFlickable();
			const bool canConnect = context.selectionCanConnect();

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("ease_type"), hasEase))
			{
				for (int i = 0; i < TXT_ARR_SZ(easeTypes); ++i)
					if (ImGui::MenuItem(getString(easeTypes[i]))) context.setEase((EaseType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("step_type"), hasStep))
			{
				for (int i = 0; i < TXT_ARR_SZ(stepTypes); ++i)
					if (ImGui::MenuItem(getString(stepTypes[i]))) context.setStep((HoldStepType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("flick_type"), hasFlick))
			{
				for (int i = 0; i < TXT_ARR_SZ(flickTypes); ++i)
					if (ImGui::MenuItem(getString(flickTypes[i]))) context.setFlick((FlickType)i);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::MenuItem(getString("shrink_up"), NULL, false, context.selectedNotes.size() > 1))
				context.shrinkSelection(Direction::Up);

			if (ImGui::MenuItem(getString("shrink_down"), NULL, false, context.selectedNotes.size() > 1))
				context.shrinkSelection(Direction::Down);

			ImGui::Separator();
			if (ImGui::MenuItem(getString("connect_holds"), NULL, false, canConnect))
				context.connectHoldsInSelection();

			if (ImGui::MenuItem(getString("split_hold"), NULL, false, hasStep && context.selectedNotes.size() == 1))
				context.splitHoldInSelection();

			ImGui::EndPopup();
		}
	}

	void ScoreEditorTimeline::update(ScoreContext& context, EditArgs& edit, Renderer* renderer)
	{
		prevSize = size;
		prevPos = position;

		// make space for the scrollbar and the status bar
		size = ImGui::GetContentRegionAvail() - ImVec2{ ImGui::GetStyle().ScrollbarSize, UI::toolbarBtnSize.y };
		position = ImGui::GetCursorScreenPos();

		boundaries = ImRect(position, position + size);
		mouseInTimeline = ImGui::IsMouseHoveringRect(position, position + size);

		laneOffset = (size.x * 0.5f) - ((NUM_LANES * laneWidth) * 0.5f);
		minOffset = size.y - 50;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(boundaries.Min, boundaries.Max, true);
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);

		if (background.isDirty())
		{
			background.resize({ size.x, size.y });
			background.process(renderer);
		}
		else if (prevSize.x != size.x || prevSize.y != size.y)
		{
			background.resize({ size.x, size.y });
		}

		if (config.drawBackground)
		{
			ImVec2 bgPos{ position };
			bgPos.x -= abs((float)background.getWidth() - size.x) / 2.0f;
			bgPos.y -= abs((float)background.getHeight() - size.y) / 2.0f;
			drawList->AddImage((ImTextureID)background.getTextureID(), bgPos, bgPos + ImVec2{ (float)background.getWidth(), (float)background.getHeight() });
		}

		// remember whether the last mouse click was in the timeline or not
		static bool clickedOnTimeline = false;
		if (ImGui::IsMouseClicked(0))
			clickedOnTimeline = mouseInTimeline;

		const bool pasting = context.pasteData.pasting;
		const ImGuiIO& io = ImGui::GetIO();
		
		// get mouse position relative to timeline
		mousePos = io.MousePos - position;
		mousePos.y -= offset;
		if (mouseInTimeline && !UI::isAnyPopupOpen())
		{
			if (io.KeyCtrl)
			{
				setZoom(zoom + (io.MouseWheel * 0.1f));
			}
			else
			{
				float scrollAmount = io.MouseWheel * scrollUnit;
				offset += scrollAmount * (io.KeyShift ? config.scrollSpeedShift : config.scrollSpeedNormal);
			}

			if (!isHoveringNote && !isHoldingNote && !insertingHold && !pasting && currentMode == TimelineMode::Select)
			{
				// clicked inside timeline, the current mouse position is the first drag point
				if (ImGui::IsMouseClicked(0))
				{
					dragStart = mousePos;
					if (!io.KeyCtrl && !io.KeyAlt && !ImGui::IsPopupOpen(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
						context.selectedNotes.clear();
				}

				// clicked and dragging inside the timeline
				if (clickedOnTimeline && ImGui::IsMouseDown(0) && ImGui::IsMouseDragPastThreshold(0, 10.0f))
					dragging = true;
			}
		}

		offset = std::max(offset, minOffset);
		updateScrollingPosition();

		// selection rectangle
		// draw selection rectangle after notes are rendered
		if (dragging && ImGui::IsMouseReleased(0) && !pasting)
		{
			// calculate drag selection
			float left = std::min(dragStart.x, mousePos.x);
			float right = std::max(dragStart.x, mousePos.x);
			float top = std::min(dragStart.y, mousePos.y);
			float bottom = std::max(dragStart.y, mousePos.y);

			if (!io.KeyAlt && !io.KeyCtrl)
				context.selectedNotes.clear();

			float yThreshold = (notesHeight * 0.5f) + 2.0f;
			for (const auto& [id, note] : context.score.notes)
			{
				float x1 = laneToPosition(note.lane);
				float x2 = laneToPosition(note.lane + note.width);
				float y = -tickToPosition(note.tick);

				if (right > x1 && left < x2 && isWithinRange(y, top - yThreshold, bottom + yThreshold))
				{
					if (io.KeyAlt)
						context.selectedNotes.erase(id);
					else
						context.selectedNotes.insert(id);
				}
			}

			dragging = false;
		}

		// draw measures
		const float x1 = getTimelineStartX();
		const float x2 = getTimelineEndX();

		drawList->AddRectFilled(
			{ x1, position.y },
			{ x2, position.y + size.y },
			Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255), 0x1c, 0x1a, 0x1f)
		);

		int firstTick = std::max(0, positionToTick(visualOffset - size.y));
		int lastTick = positionToTick(visualOffset);
		int measure = accumulateMeasures(firstTick, TICKS_PER_BEAT, context.score.timeSignatures);
		firstTick = measureToTicks(measure, TICKS_PER_BEAT, context.score.timeSignatures);

		int tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		int ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
		int beatTicks = ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator; 
		int subdivision = TICKS_PER_BEAT / (division / 4);

		// snap to the sub-division before the current measure to prevent the lines from jumping around
		for (int tick = firstTick - (firstTick % subdivision); tick <= lastTick; tick += subdivision)
		{
			const int y = position.y - tickToPosition(tick) + visualOffset;
			int currentMeasure = accumulateMeasures(tick, TICKS_PER_BEAT, context.score.timeSignatures);

			// time signature changes on current measure
			if (context.score.timeSignatures.find(currentMeasure) != context.score.timeSignatures.end() && currentMeasure != tsIndex)
			{
				tsIndex = currentMeasure;
				ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
				beatTicks = ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;

				// snap to sub-division again on time signature change
				tick = measureToTicks(currentMeasure, TICKS_PER_BEAT, context.score.timeSignatures);
				tick -= tick % subdivision;
			}

			// determine whether the tick is a beat relative to its measure's tick
			int measureTicks = measureToTicks(currentMeasure, TICKS_PER_BEAT, context.score.timeSignatures);

			if (!((tick - measureTicks) % beatTicks))
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), measureColor, primaryLineThickness);
			else if (division < 192)
				drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), divColor2, secondaryLineThickness);
		}

		tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;

		// overdraw one measure to make sure the measure string is always visible
		for (int tick = firstTick; tick < lastTick + ticksPerMeasure; tick += ticksPerMeasure)
		{
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
			{
				tsIndex = measure;
				ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
			}

			std::string measureStr = std::to_string(measure);
			const float txtPos = x1 - MEASURE_WIDTH - (ImGui::CalcTextSize(measureStr.c_str()).x * 0.5f);
			const int y = position.y - tickToPosition(tick) + visualOffset;

			drawList->AddLine(ImVec2(x1 - MEASURE_WIDTH, y), ImVec2(x2 + MEASURE_WIDTH, y), measureColor, primaryLineThickness);
			drawShadedText(drawList, ImVec2( txtPos, y), 26, measureTxtColor, measureStr.c_str());

			++measure;
		}

		// draw lanes
		for (int l = 0; l <= NUM_LANES; ++l)
		{
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y), boldLane ? divColor1 : divColor2, boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		hoverTick = snapTickFromPos(-mousePos.y, context);
		hoverLane = positionToLane(mousePos.x);
		isHoveringNote = false;

		// draw cursor behind notes
		const int y = position.y - tickToPosition(context.currentTick) + visualOffset;
		const int triPtOffset = 6;
		const int triXPos = x1 - (triPtOffset * 2);
		drawList->AddTriangleFilled(ImVec2(triXPos, y - triPtOffset), ImVec2(triXPos, y + triPtOffset), ImVec2(triXPos + (triPtOffset * 2), y), cursorColor);
		drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), cursorColor, primaryLineThickness + 1.0f);

		contextMenu(context);

		// update hi-speed changes
		for (int index = 0; index < context.score.hiSpeedChanges.size(); ++index)
		{
			HiSpeedChange& hiSpeed = context.score.hiSpeedChanges[index];
			if (hiSpeedControl(hiSpeed))
			{
				eventEdit.editIndex = index;
				eventEdit.editHiSpeed = hiSpeed.speed;
				eventEdit.type = EventType::HiSpeed;
				ImGui::OpenPopup("edit_event");
			}
		}

		// update time signature changes
		for (auto& [measure, ts] : context.score.timeSignatures)
		{
			if (timeSignatureControl(ts.numerator, ts.denominator, measureToTicks(ts.measure, TICKS_PER_BEAT, context.score.timeSignatures), !playing))
			{
				eventEdit.editIndex = measure;
				eventEdit.editTimeSignatureNumerator = ts.numerator;
				eventEdit.editTimeSignatureDenominator = ts.denominator;
				eventEdit.type = EventType::TimeSignature;
				ImGui::OpenPopup("edit_event");
			}
		}

		// update bpm changes
		for (int index = 0; index < context.score.tempoChanges.size(); ++index)
		{
			Tempo& tempo = context.score.tempoChanges[index];
			if (bpmControl(tempo))
			{
				eventEdit.editIndex = index;
				eventEdit.editBpm = tempo.bpm;
				eventEdit.type = EventType::Bpm;
				ImGui::OpenPopup("edit_event");
			}
		}

		feverControl(context.score.fever);

		// update skill triggers
		for (const auto& skill : context.score.skills)
			skillControl(skill);

		eventEditor(context);
		updateNotes(context, edit, renderer);

		// update cursor tick after determining whether a note is hovered
		// the cursor tick should not change if a note is hovered
		if (ImGui::IsMouseClicked(0) && !isHoveringNote && mouseInTimeline && !playing && !pasting &&
			!UI::isAnyPopupOpen() && currentMode == TimelineMode::Select && ImGui::IsWindowFocused())
		{
			context.currentTick = hoverTick;
			lastSelectedTick = context.currentTick;
		}

		// selection boxes
		for (int id : context.selectedNotes)
		{
			const Note& note = context.score.notes.at(id);
			if (!isNoteVisible(note, 0))
				continue;

			float x = position.x;
			float y = position.y - tickToPosition(note.tick) + visualOffset;

			ImVec2 p1{ x + laneToPosition(note.lane) - 3, y - (notesHeight * 0.5f)};
			ImVec2 p2{ x + laneToPosition(note.lane + note.width) + 3, y + (notesHeight * 0.5f)};

			drawList->AddRectFilled(p1, p2, 0x20f4f4f4, 2.0f, ImDrawFlags_RoundCornersAll);
			drawList->AddRect(p1, p2, 0xcccccccc, 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
		}

		if (dragging && !pasting)
		{
			float startX = std::min(position.x + dragStart.x, position.x + mousePos.x);
			float endX = std::max(position.x + dragStart.x, position.x + mousePos.x);
			float startY = std::min(position.y + dragStart.y, position.y + mousePos.y) + visualOffset;
			float endY = std::max(position.y + dragStart.y, position.y + mousePos.y) + visualOffset;
			ImVec2 start{ startX, startY };
			ImVec2 end{ endX, endY };

			drawList->AddRectFilled(start, end, selectionColor1);
			drawList->AddRect(start, end, 0xbbcccccc, 0.2f, ImDrawFlags_RoundCornersAll, 1.0f);

			ImVec2 iconPos = ImVec2(position + dragStart);
			iconPos.y += visualOffset;
			if (io.KeyCtrl)
			{
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_PLUS_CIRCLE);
			}
			else if (io.KeyAlt)
			{
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd, ICON_FA_MINUS_CIRCLE);
			}
		}

		drawList->PopClipRect();

		// status bar: playback controls, division, zoom, current time and rhythm
		ImGui::SetCursorPos(ImVec2{ ImGui::GetStyle().WindowPadding.x, size.y + UI::toolbarBtnSize.y + ImGui::GetStyle().WindowPadding.y });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

		ImGui::PushButtonRepeat(true);
		if (UI::toolbarButton(ICON_FA_BACKWARD, "", NULL, !playing && context.currentTick > 0))
			previousTick(context);
		ImGui::PopButtonRepeat();

		ImGui::SameLine();
		if (UI::toolbarButton(ICON_FA_STOP, "", NULL))
			stop(context);

		ImGui::SameLine();
		if (UI::toolbarButton(playing ? ICON_FA_PAUSE : ICON_FA_PLAY, "", NULL))
			togglePlaying(context);

		ImGui::SameLine();
		ImGui::PushButtonRepeat(true);
		if (UI::toolbarButton(ICON_FA_FORWARD, "", NULL, !playing))
			nextTick(context);
		ImGui::PopButtonRepeat();

		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		UI::divisionSelect(getString("division"), division, divisions, sizeof(divisions) / sizeof(int));

		static int gotoMeasure = 0;
		bool activated = false;

		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		ImGui::InputInt("##goto_measure", &gotoMeasure, 0, 0, ImGuiInputTextFlags_AutoSelectAll);
		activated |= ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
		UI::tooltip(getString("goto_measure"));

		ImGui::SameLine();
		activated |= UI::transparentButton(ICON_FA_ARROW_RIGHT, UI::btnSmall);

		if (activated)
		{
			gotoMeasure = std::clamp(gotoMeasure, 0, 999);
			context.currentTick = measureToTicks(gotoMeasure, TICKS_PER_BEAT, context.score.timeSignatures);
			offset = std::max(minOffset, tickToPosition(context.currentTick) + (size.y * (1.0f - config.cursorPositionThreshold)));
		}

		ImGui::SameLine();
		float _zoom = zoom;
		if (UI::zoomControl("zoom", _zoom, MIN_ZOOM, 10.0f))
			setZoom(_zoom);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		int currentMeasure = accumulateMeasures(context.currentTick, TICKS_PER_BEAT, context.score.timeSignatures);
		const TimeSignature& ts = context.score.timeSignatures[findTimeSignature(currentMeasure, context.score.timeSignatures)];
		const Tempo& tempo = getTempoAt(context.currentTick, context.score.tempoChanges);

		int hiSpeed = findHighSpeedChange(context.currentTick, context.score.hiSpeedChanges);
		float speed = (hiSpeed == -1 ? 1.0f : context.score.hiSpeedChanges[hiSpeed].speed);

		ImGui::Text(IO::formatString(
			"  %02d:%02d:%02d  |  %d/%d  |  %g BPM  |  %gx",
			(int)time / 60, (int)time % 60, (int)((time - (int)time) * 100),
			ts.numerator, ts.denominator,
			tempo.bpm,
			speed
		).c_str());

		updateScrollbar();

		updateNoteSE(context);

		timeLastFrame = time;
		if (playing)
		{
			time += ImGui::GetIO().DeltaTime;
			context.currentTick = accumulateTicks(time, TICKS_PER_BEAT, context.score.tempoChanges);

			float cursorY = tickToPosition(context.currentTick);
			if (config.followCursorInPlayback)
			{
				float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
				if (cursorY >= offset - timelineOffset)
					visualOffset = offset = cursorY + timelineOffset;
			}
			else if (cursorY > offset)
			{
				visualOffset = offset = cursorY + size.y;	
			}
		}
		else
		{
			time = accumulateDuration(context.currentTick, TICKS_PER_BEAT, context.score.tempoChanges);
		}
	}

	void ScoreEditorTimeline::updateNotes(ScoreContext& context, EditArgs& edit, Renderer* renderer)
	{
		// directxmath dies
		if (size.y < 10 || size.x < 10) return;

		Shader* shader = ResourceManager::shaders[0];
		shader->use();
		shader->setMatrix4("projection", camera.getOffCenterOrthographicProjection(0, size.x, position.y, position.y + size.y));

		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		minNoteYDistance = INT_MAX;
		for (auto& [id, note] : context.score.notes)
		{
			if (isNoteVisible(note) && note.getType() == NoteType::Tap)
			{
				updateNote(context, note);
				drawNote(note, renderer, noteTint);
			}
		}

		for (auto& [id, hold] : context.score.holdNotes)
		{
			Note& start = context.score.notes.at(hold.start.ID);
			Note& end = context.score.notes.at(hold.end);

			if (isNoteVisible(start)) updateNote(context, start);
			if (isNoteVisible(end)) updateNote(context, end);

			for (const auto& step : hold.steps)
			{
				Note& mid = context.score.notes.at(step.ID);
				if (isNoteVisible(mid)) updateNote(context, mid);
				if (skipUpdateAfterSortingSteps) break;
			}

			drawHoldNote(context.score.notes, hold, renderer, noteTint);
		}
		skipUpdateAfterSortingSteps = false;

		renderer->endBatch();
		renderer->beginBatch();

		const bool pasting = context.pasteData.pasting;
		if (pasting && mouseInTimeline)
		{
			context.pasteData.offsetTicks = hoverTick;
			context.pasteData.offsetLane = hoverLane;
			previewPaste(context, renderer);
			if (ImGui::IsMouseClicked(0))
				context.confirmPaste();
			else if (ImGui::IsMouseClicked(1))
				context.cancelPaste();
		}

		if (mouseInTimeline && !isHoldingNote && currentMode != TimelineMode::Select &&
			!pasting && !UI::isAnyPopupOpen())
		{
			previewInput(edit, renderer);
			if (ImGui::IsMouseClicked(0) && hoverTick >= 0 && !isHoveringNote)
				executeInput(context, edit);

			if (insertingHold && !ImGui::IsMouseDown(0))
			{
				insertHold(context, edit);
				insertingHold = false;
			}
		}
		else
		{
			insertingHold = false;
		}

		renderer->endBatch();

		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddImage((void*)framebuffer->getTexture(), position, position + size);

		// draw hold step outlines
		for (const auto& data : drawSteps)
			drawOutline(data);

		drawSteps.clear();
	}

	void ScoreEditorTimeline::previewPaste(ScoreContext& context, Renderer* renderer)
	{
		context.pasteData.offsetLane = std::clamp(hoverLane - context.pasteData.midLane,
			context.pasteData.minLaneOffset,
			context.pasteData.maxLaneOffset);

		for (const auto& [_, note] : context.pasteData.notes)
			if (note.getType() == NoteType::Tap && isNoteVisible(note, hoverTick))
				drawNote(note, renderer, hoverTint, hoverTick, context.pasteData.offsetLane);

		for (const auto& [_, hold] : context.pasteData.holds)
			drawHoldNote(context.pasteData.notes, hold, renderer, hoverTint, hoverTick, context.pasteData.offsetLane);
	}

	void ScoreEditorTimeline::updateInputNotes(EditArgs& edit)
	{
		int lane = laneFromCenterPosition(hoverLane, edit.noteWidth);
		int width = edit.noteWidth;
		int tick = hoverTick;

		inputNotes.tap.lane = lane;
		inputNotes.tap.width = width;
		inputNotes.tap.tick = tick;
		inputNotes.tap.flick = currentMode == TimelineMode::InsertFlick ? edit.flickType : FlickType::None;
		inputNotes.tap.critical = currentMode == TimelineMode::MakeCritical;
		inputNotes.tap.friction = currentMode == TimelineMode::MakeFriction;

		inputNotes.holdStep.lane = lane;
		inputNotes.holdStep.width = width;
		inputNotes.holdStep.tick = tick;

		if (insertingHold)
		{
			inputNotes.holdEnd.lane = lane;
			inputNotes.holdEnd.width = width;
			inputNotes.holdEnd.tick = tick;
		}
		else
		{
			inputNotes.holdStart.lane = lane;
			inputNotes.holdStart.width = width;
			inputNotes.holdStart.tick = tick;
		}
	}

	void ScoreEditorTimeline::insertEvent(ScoreContext& context, EditArgs& edit)
	{
		if (currentMode == TimelineMode::InsertBPM)
		{
			for (const auto& tempo : context.score.tempoChanges)
				if (tempo.tick == hoverTick)
					return;

			Score prev = context.score;
			context.score.tempoChanges.push_back({ hoverTick, edit.bpm });
			Utilities::sort<Tempo>(context.score.tempoChanges, [](const Tempo& a, const Tempo& b) { return a.tick < b.tick; });

			context.pushHistory("Insert BPM change", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertTimeSign)
		{
			int measure = accumulateMeasures(hoverTick, TICKS_PER_BEAT, context.score.timeSignatures);
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
				return;

			Score prev = context.score;
			context.score.timeSignatures[measure] = { measure, edit.timeSignatureNumerator, edit.timeSignatureDenominator };
			context.pushHistory("Insert time signature", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertHiSpeed)
		{
			for (const auto& hs : context.score.hiSpeedChanges)
				if (hs.tick == hoverTick)
					return;

			Score prev = context.score;
			context.score.hiSpeedChanges.push_back({ hoverTick, edit.hiSpeed });
			context.pushHistory("Insert hi-speed changes", prev, context.score);
		}
	}

	void ScoreEditorTimeline::previewInput(EditArgs& edit, Renderer* renderer)
	{
		updateInputNotes(edit);
		switch (currentMode)
		{
		case TimelineMode::InsertLong:
			if (insertingHold)
			{
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear, renderer, noteTint);
				drawNote(inputNotes.holdStart, renderer, noteTint);
				drawNote(inputNotes.holdEnd, renderer, noteTint);
			}
			else
			{
				drawNote(inputNotes.holdStart, renderer, hoverTint);
			}
			break;

		case TimelineMode::InsertLongMid:
			drawHoldMid(inputNotes.holdStep, edit.stepType, renderer, hoverTint);
			drawOutline(StepDrawData{ inputNotes.holdStep.tick, inputNotes.holdStep.lane, inputNotes.holdStep.width, edit.stepType });
			break;

		case TimelineMode::InsertBPM:
			bpmControl(edit.bpm, hoverTick, false);
			break;

		case TimelineMode::InsertTimeSign:
			timeSignatureControl(edit.timeSignatureNumerator, edit.timeSignatureDenominator, hoverTick, false);
			break;

		case TimelineMode::InsertHiSpeed:
			hiSpeedControl(hoverTick, edit.hiSpeed);
			break;

		default:
			drawNote(inputNotes.tap, renderer, hoverTint);
			break;
		}
	}

	void ScoreEditorTimeline::executeInput(ScoreContext& context, EditArgs& edit)
	{
		switch (currentMode)
		{
		case TimelineMode::InsertTap:
		case TimelineMode::MakeCritical:
		case TimelineMode::MakeFriction:
		case TimelineMode::InsertFlick:
			insertNote(context, edit, currentMode == TimelineMode::MakeCritical);
			break;

		case TimelineMode::InsertLong:
			insertingHold = true;
			break;

		case TimelineMode::InsertLongMid:
		{
			int id = findClosestHold(context, hoverLane, hoverTick);
			if (id != -1)
				insertHoldStep(context, edit, id);
		}
		break;

		case TimelineMode::InsertBPM:
		case TimelineMode::InsertTimeSign:
		case TimelineMode::InsertHiSpeed:
			insertEvent(context, edit);
			break;

		default:
			break;
		}
	}

	void ScoreEditorTimeline::changeMode(TimelineMode mode, EditArgs& edit)
	{
		if (currentMode == mode)
		{
			if (mode == TimelineMode::InsertLongMid)
			{
				edit.stepType = (HoldStepType)(((int)edit.stepType + 1) % (int)HoldStepType::HoldStepTypeCount);
			}
			else if (mode == TimelineMode::InsertFlick)
			{
				edit.flickType = (FlickType)(((int)edit.flickType + 1) % (int)FlickType::FlickTypeCount);
				if (!(int)edit.flickType)
					edit.flickType = FlickType::Default;
			}
		}

		currentMode = mode;
	}

	int ScoreEditorTimeline::findClosestHold(ScoreContext& context, int lane, int tick)
	{
		float xt = laneToPosition(lane);
		float yt = getNoteYPosFromTick(tick);

		for (auto& [id, hold] : context.score.holdNotes)
		{
			const Note& start = context.score.notes.at(hold.start.ID);
			const Note& end = context.score.notes.at(hold.end);

			if (hold.steps.size())
			{
				const HoldStep& mid1 = hold.steps[0];
				if (isMouseInHoldPath(start, context.score.notes.at(mid1.ID), hold.start.ease, xt, yt))
					return id;

				for (int step = 0; step < hold.steps.size() - 1; ++step)
				{
					const Note& m1 = context.score.notes.at(hold.steps[step].ID);
					const Note& m2 = context.score.notes.at(hold.steps[step + 1].ID);
					if (isMouseInHoldPath(m1, m2, hold.steps[step].ease, xt, yt))
						return id;
				}

				const Note& lastMid = context.score.notes.at(hold.steps[hold.steps.size() - 1].ID);
				if (isMouseInHoldPath(lastMid, end, hold.steps[hold.steps.size() - 1].ease, xt, yt))
					return id;
			}
			else
			{
				if (isMouseInHoldPath(start, end, hold.start.ease, xt, yt))
					return id;
			}
		}

		return -1;
	}

	bool ScoreEditorTimeline::noteControl(ScoreContext& context, const ImVec2& pos, const ImVec2& sz, const char* id, ImGuiMouseCursor cursor)
	{
		// do not process notes if the cursor is outside of the timeline
		// this fixes ui buttons conflicting with note "buttons"
		if (!mouseInTimeline && !isHoldingNote)
			return false;

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, sz);
		if (mouseInTimeline && ImGui::IsItemHovered() && !dragging)
			ImGui::SetMouseCursor(cursor);

		// note clicked
		if (ImGui::IsItemActivated())
		{
			prevUpdateScore = context.score;
			ctrlMousePos = mousePos;
			holdLane = hoverLane;
			holdTick = hoverTick;
		}

		// holding note
		if (ImGui::IsItemActive())
		{
			ImGui::SetMouseCursor(cursor);
			isHoldingNote = true;
			return true;
		}

		// note released
		if (ImGui::IsItemDeactivated())
		{
			isHoldingNote = false;
			if (hasEdit)
			{
				std::unordered_set<int> sortHolds = context.getHoldsFromSelection();
				for (int id : sortHolds)
				{
					HoldNote& hold = context.score.holdNotes.at(id);
					Note& start = context.score.notes.at(id);
					Note& end = context.score.notes.at(hold.end);

					if (start.tick > end.tick)
					{
						std::swap(start.tick, end.tick);
						std::swap(start.lane, end.lane);
						std::swap(start.width, end.width);
					}

					if (hold.steps.size())
					{
						sortHoldSteps(context.score, hold);

						// ensure hold steps are between the start and end
						Note& firstMid = context.score.notes.at(hold.steps[0].ID);
						if (start.tick > firstMid.tick)
						{
							std::swap(start.tick, firstMid.tick);
							std::swap(start.lane, firstMid.lane);
						}

						Note& lastMid = context.score.notes.at(hold.steps[hold.steps.size() - 1].ID);
						if (end.tick < lastMid.tick)
						{
							std::swap(end.tick, lastMid.tick);
							std::swap(end.lane, lastMid.lane);
						}
					}

					sortHoldSteps(context.score, hold);
					skipUpdateAfterSortingSteps = true;
				}

				context.pushHistory("Update notes", prevUpdateScore, context.score);
				hasEdit = false;
			}
		}

		return false;
	}

	void ScoreEditorTimeline::updateNote(ScoreContext& context, Note& note)
	{
		const float btnPosY = position.y - tickToPosition(note.tick) + visualOffset - (notesHeight * 0.5f);
		float btnPosX = laneToPosition(note.lane) + position.x - 2.0f;

		ImVec2 pos{ btnPosX, btnPosY };
		ImVec2 noteSz{ laneToPosition(note.lane + note.width) + position.x + 2.0f - btnPosX, notesHeight };
		ImVec2 sz{ noteControlWidth, notesHeight };

		const ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsMouseHoveringRect(pos, pos + noteSz, false) && mouseInTimeline)
		{
			isHoveringNote = true;

			float noteYDistance = std::abs((btnPosY + notesHeight / 2 - visualOffset - position.y) - mousePos.y);
			if (noteYDistance < minNoteYDistance || io.KeyCtrl)
			{
				minNoteYDistance = noteYDistance;
				hoveringNote = note.ID;
				if (ImGui::IsMouseClicked(0) && !UI::isAnyPopupOpen())
				{
					if (!io.KeyCtrl && !io.KeyAlt && !context.isNoteSelected(note))
						context.selectedNotes.clear();

					context.selectedNotes.insert(note.ID);

					if (io.KeyAlt && context.isNoteSelected(note))
						context.selectedNotes.erase(note.ID);
				}
			}
		}

		// left resize
		ImGui::PushID(note.ID);
		if (noteControl(context, pos, sz, "L", ImGuiMouseCursor_ResizeEW))
		{
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int diff = curLane - grabLane;

			if (abs(diff) > 0)
			{
				bool canResize = true;
				for (int id : context.selectedNotes)
				{
					Note& n = context.score.notes.at(id);
					int newLane = n.lane + diff;
					int newWidth = n.width - diff;

					if (newLane < MIN_LANE || newLane + newWidth - 1 > MAX_LANE || newWidth < MIN_NOTE_WIDTH || newWidth > MAX_NOTE_WIDTH)
					{
						canResize = false;
						break;
					}
				}

				if (canResize)
				{
					ctrlMousePos.x = mousePos.x;
					hasEdit = true;
					for (int id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.width = std::clamp(n.width - diff, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH);
						n.lane = std::clamp(n.lane + diff, MIN_LANE, MAX_LANE - n.width + 1);
					}
				}
			}
		}

		pos.x += noteControlWidth;
		sz.x = (laneWidth * note.width) + 4.0f - (noteControlWidth * 2.0f);

		// move
		if (noteControl(context, pos, sz, "M", ImGuiMouseCursor_ResizeAll))
		{
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int grabTick = snapTickFromPos(-ctrlMousePos.y, context);

			int diff = curLane - grabLane;
			if (abs(diff) > 0)
			{
				isMovingNote = true;
				ctrlMousePos.x = mousePos.x;

				bool canMove = true;
				for (int id : context.selectedNotes)
				{
					Note& n = context.score.notes.at(id);
					int newLane = n.lane + diff;

					if (newLane < MIN_LANE || newLane + n.width - 1 > MAX_LANE)
					{
						canMove = false;
						break;
					}
				}

				if (canMove)
				{
					hasEdit = true;
					for (int id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.lane = std::clamp(n.lane + diff, MIN_LANE, MAX_LANE - n.width + 1);
					}
				}
			}

			diff = hoverTick - grabTick;
			if (abs(diff) > 0)
			{
				isMovingNote = true;
				ctrlMousePos.y = mousePos.y;

				bool canMove = true;
				for (int id : context.selectedNotes)
				{
					Note& n = context.score.notes.at(id);
					int newTick = n.tick + diff;

					if (newTick < 0)
					{
						canMove = false;
						break;
					}
				}

				if (canMove)
				{
					hasEdit = true;
					for (int id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.tick = std::max(n.tick + diff, 0);
					}
				}
			}
		}

		// per note options here
		if (ImGui::IsItemDeactivated())
		{
			if (!isMovingNote && context.selectedNotes.size())
			{
				switch (currentMode)
				{
				case TimelineMode::InsertFlick:
					context.setFlick(FlickType::FlickTypeCount);
					break;

				case TimelineMode::MakeCritical:
					context.toggleCriticals();
					break;

				case TimelineMode::InsertLong:
					context.setEase(EaseType::EaseTypeCount);
					break;

				case TimelineMode::InsertLongMid:
					context.setStep(HoldStepType::HoldStepTypeCount);
					break;

				case TimelineMode::MakeFriction:
					context.toggleFriction();
					break;

				default:
					break;
				}
			}

			isMovingNote = false;
		}

		pos.x += sz.x;
		sz.x = noteControlWidth;

		// right resize
		if (noteControl(context, pos, sz, "R", ImGuiMouseCursor_ResizeEW))
		{
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int curLane = positionToLane(mousePos.x);

			int diff = curLane - grabLane;
			if (abs(diff) > 0)
			{
				bool canResize = true;
				for (int id : context.selectedNotes)
				{
					Note& n = context.score.notes.at(id);
					int newWidth = n.width + diff;

					if (newWidth < MIN_NOTE_WIDTH || n.lane + newWidth - 1 > MAX_LANE)
					{
						canResize = false;
						break;
					}
				}

				if (canResize)
				{
					ctrlMousePos.x = mousePos.x;
					hasEdit = true;
					for (int id : context.selectedNotes)
					{
						Note& n = context.score.notes.at(id);
						n.width = std::clamp(n.width + diff, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH - n.lane);
					}
				}
			}
		}

		ImGui::PopID();
	}

	void ScoreEditorTimeline::drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		if (noteTextures.holdPath == -1)
			return;

		const Texture& pathTex = ResourceManager::textures[noteTextures.holdPath];
		const int sprIndex = n1.critical ? 3 : 1;
		if (sprIndex > pathTex.sprites.size())
			return;

		const Sprite& spr = pathTex.sprites[sprIndex];

		float startX1 = laneToPosition(n1.lane + offsetLane);
		float startX2 = laneToPosition(n1.lane + n1.width + offsetLane);
		float startY = getNoteYPosFromTick(n1.tick + offsetTick);

		float endX1 = laneToPosition(n2.lane + offsetLane);
		float endX2 = laneToPosition(n2.lane + n2.width + offsetLane);
		float endY = getNoteYPosFromTick(n2.tick + offsetTick);

		int left = spr.getX() + holdCutoffX;
		int right = spr.getX() + spr.getWidth() - holdCutoffX;

		float steps = ease == EaseType::Linear ? 1 : std::max(5.0f, std::ceilf(abs((endY - startY)) / 10));
		for (int y = 0; y < steps; ++y)
		{
			const float percent1 = y / steps;
			const float percent2 = (y + 1) / steps;
			float i1 = ease == EaseType::Linear ? percent1 : ease == EaseType::EaseIn ? easeIn(percent1) : easeOut(percent1);
			float i2 = ease == EaseType::Linear ? percent2 : ease == EaseType::EaseIn ? easeIn(percent2) : easeOut(percent2);

			float xl1 = lerp(startX1, endX1, i1) - 2;
			float xr1 = lerp(startX2, endX2, i1) + 2;
			float y1 = lerp(startY, endY, percent1);
			float y2 = lerp(startY, endY, percent2);
			float xl2 = lerp(startX1, endX1, i2) - 2;
			float xr2 = lerp(startX2, endX2, i2) + 2;

			if (y2 <= 0)
				continue;

			// rest of hold no longer visible
			if (y1 > size.y + size.y + position.y + 100)
				break;

			Vector2 p1{ xl1, y1 };
			Vector2 p2{ xl1 + holdSliceSize, y1 };
			Vector2 p3{ xl2, y2 };
			Vector2 p4{ xl2 + holdSliceSize, y2 };
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left, left + holdSliceWidth,
				spr.getY(), spr.getY() + spr.getHeight(), tint);

			p1.x = xl1 + holdSliceSize;
			p2.x = xr1 - holdSliceSize;
			p3.x = xl2 + holdSliceSize;
			p4.x = xr2 - holdSliceSize;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left + holdSliceWidth, right - holdSliceWidth,
				spr.getY(), spr.getY() + spr.getHeight(), tint);

			p1.x = xr1 - holdSliceSize;
			p2.x = xr1;
			p3.x = xr2 - holdSliceSize;
			p4.x = xr2;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, right - holdSliceWidth, right,
				spr.getY(), spr.getY() + spr.getHeight(), tint);
		}
	}

	void ScoreEditorTimeline::drawInputNote(Renderer* renderer)
	{
		if (insertingHold)
		{
			drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear, renderer, noteTint);
			drawNote(inputNotes.holdStart, renderer, noteTint);
			drawNote(inputNotes.holdEnd, renderer, noteTint);
		}
		else
		{
			drawNote(currentMode == TimelineMode::InsertLong ? inputNotes.holdStart : inputNotes.tap, renderer, hoverTint);
		}
	}

	void ScoreEditorTimeline::drawHoldNote(const std::unordered_map<int, Note>& notes, const HoldNote& note, Renderer* renderer,
		const Color& tint, const int offsetTicks, const int offsetLane)
	{
		const Note& start = notes.at(note.start.ID);
		const Note& end = notes.at(note.end);
		if (note.steps.size())
		{
			static constexpr auto isSkipStep = [](const HoldStep& step) { return step.type == HoldStepType::Skip; };
			int s1 = -1;
			int s2 = -1;

			for (int i = 0; i < note.steps.size(); ++i)
			{
				if (isSkipStep(note.steps[i]))
					continue;

				s2 = i;
				const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
				const Note& n2 = s2 == -1 ? end : notes.at(note.steps[s2].ID);
				const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
				drawHoldCurve(n1, n2, ease, renderer, tint, offsetTicks, offsetLane);

				s1 = s2;
			}

			const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
			const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
			drawHoldCurve(n1, end, ease, renderer, tint, offsetTicks, offsetLane);

			s1 = -1;
			s2 = 1;

			if (noteTextures.notes == -1)
				return;

			const Texture& tex = ResourceManager::textures[noteTextures.notes];
			const Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };
			for (int i = 0; i < note.steps.size(); ++i)
			{
				const Note& n3 = notes.at(note.steps[i].ID);

				// find first non-skip step
				s2 = std::distance(note.steps.cbegin(), std::find_if(note.steps.cbegin() + std::max(s2, i), note.steps.cend(),
					std::not_fn(isSkipStep)));

				if (isNoteVisible(n3, offsetTicks))
				{
					if (drawHoldStepOutlines)
						drawSteps.emplace_back(StepDrawData{ n3.tick + offsetTicks, n3.lane + offsetLane, n3.width, note.steps[i].type });

					if (note.steps[i].type != HoldStepType::Hidden)
					{
						int sprIndex = getNoteSpriteIndex(n3);
						if (sprIndex > -1 && sprIndex < tex.sprites.size())
						{
							const Sprite& s = tex.sprites[sprIndex];
							Vector2 pos{ laneToPosition(n3.lane + offsetLane + (n3.width / 2.0f)), getNoteYPosFromTick(n3.tick + offsetTicks) };

							if (isSkipStep(note.steps[i]))
							{
								// find the note before and after the skip step
								const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
								const Note& n2 = s2 >= note.steps.size() ? end : notes.at(note.steps[s2].ID);

								// calculate the interpolation ratio based on the distance between n1 and n2
								float ratio = (float)(n3.tick - n1.tick) / (float)(n2.tick - n1.tick);
								const EaseType rEase = s1 == -1 ? note.start.ease : note.steps[s1].ease;
								if (rEase == EaseType::EaseIn)
								{
									ratio = easeIn(ratio);
								}
								else if (rEase == EaseType::EaseOut)
								{
									ratio = easeOut(ratio);
								}

								// interpolate the step's position
								float x1 = lerp(laneToPosition(n1.lane + offsetLane), laneToPosition(n2.lane + offsetLane), ratio);
								float x2 = lerp(laneToPosition(n1.lane + offsetLane + n1.width), laneToPosition(n2.lane + offsetLane + n2.width), ratio);
								pos.x = (x1 + x2) / 2.0f;
							}

							renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, s.getX(), s.getX() + s.getWidth(),
								s.getY(), s.getY() + s.getHeight(), tint, 1);
						}
					}
				}

				// update first interpolation point
				if (!isSkipStep(note.steps[i]))
					s1 = i;
			}
		}
		else
		{
			drawHoldCurve(start, end, note.start.ease, renderer, tint, offsetTicks, offsetLane);
		}

		if (isNoteVisible(start, offsetTicks))
			drawNote(start, renderer, tint, offsetTicks, offsetLane);

		if (isNoteVisible(end, offsetTicks))
			drawNote(end, renderer, tint, offsetTicks, offsetLane);
	}

	void ScoreEditorTimeline::drawHoldMid(Note& note, HoldStepType type, Renderer* renderer, const Color& tint)
	{
		if (type == HoldStepType::Hidden || noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		int sprIndex = getNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		// center diamond if width is even
		Vector2 pos{ laneToPosition(note.lane + (note.width / 2.0f)), getNoteYPosFromTick(note.tick) };
		Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };

		renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter,
			tex, s.getX(), s.getX() + s.getWidth(), s.getY(), s.getY() + s.getHeight(), tint, 1);
	}

	void ScoreEditorTimeline::drawOutline(const StepDrawData& data)
	{
		float x = position.x;
		float y = position.y - tickToPosition(data.tick) + visualOffset;

		ImVec2 p1{ x + laneToPosition(data.lane), y - (notesHeight * 0.15f) };
		ImVec2 p2{ x + laneToPosition(data.lane + data.width), y + (notesHeight * 0.15f) };

		ImU32 fill = data.type == HoldStepType::Skip ? 0x55ffffaa : 0x55aaffaa;
		ImU32 outline = data.type == HoldStepType::Skip ? 0xffffffaa : 0xffccffaa;
		ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, fill, 2.0f, ImDrawFlags_RoundCornersAll);
		ImGui::GetWindowDrawList()->AddRect(p1, p2, outline, 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
	}

	void ScoreEditorTimeline::drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getFlickArrowSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& arrowS = tex.sprites[sprIndex];

		Vector2 pos{ 0, getNoteYPosFromTick(note.tick + offsetTick) };
		const float x1 = laneToPosition(note.lane + offsetLane);
		const float x2 = pos.x + laneToPosition(note.lane + note.width + offsetLane);
		pos.x = (x1 + x2) * 0.5f; // get note middle point
		pos.y += notesHeight * 0.7f; // move the arrow up a bit

		// notes wider than 6 lanes also use flick arrow size 6
		int sizeIndex = std::min(note.width - 1, 5);
		Vector2 size{ laneWidth * flickArrowWidths[sizeIndex], notesHeight * flickArrowHeights[sizeIndex] };

		float sx1 = arrowS.getX();
		float sx2 = arrowS.getX() + arrowS.getWidth();
		if (note.flick == FlickType::Right)
		{
			// flip arrow to point to the right
			sx1 = arrowS.getX() + arrowS.getWidth();
			sx2 = arrowS.getX();
		}

		renderer->drawSprite(pos, 0.0f, size, AnchorType::MiddleCenter, tex,
			sx1, sx2, arrowS.getY(), arrowS.getY() + arrowS.getHeight(), tint, 2);
	}

	void ScoreEditorTimeline::drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick, const int offsetLane)
	{
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		Vector2 pos{ laneToPosition(note.lane + offsetLane), getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen = (laneWidth * note.width) - (sliceSz.x * 2) + noteOffsetX + 5;
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		// left slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
			left, left + noteSliceWidth,
			s.getY(), s.getY() + s.getHeight(), tint, 1
		);
		pos.x += sliceSz.x;

		// middle
		renderer->drawSprite(pos, 0.0f, midSz, anchor, tex,
			left + noteSliceWidth, right - noteSliceWidth,
			s.getY(), s.getY() + s.getHeight(), tint, 1
		);
		pos.x += midLen;

		// right slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex,
			right - noteSliceWidth, right,
			s.getY(), s.getY() + s.getHeight(), tint, 1
		);

		if (note.friction)
		{
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (frictionSprIndex >= 0 && frictionSprIndex < tex.sprites.size())
			{
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight, notesHeight };
				pos.x = (laneToPosition(note.lane + offsetLane) + laneToPosition(note.lane + offsetLane + note.width)) / 2.0f;
				renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(), frictionSpr.getX() + frictionSpr.getWidth(),
					frictionSpr.getY(), frictionSpr.getY() + frictionSpr.getHeight(), tint, 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
	}

	bool ScoreEditorTimeline::bpmControl(const Tempo& tempo)
	{
		return bpmControl(tempo.bpm, tempo.tick, !playing);
	}

	bool ScoreEditorTimeline::bpmControl(float bpm, int tick, bool enabled)
	{
		Vector2 pos{ getTimelineEndX() + 10, position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineEndX(), pos, tempoColor, IO::formatString("%g BPM", bpm).c_str(), enabled);
	}

	bool ScoreEditorTimeline::timeSignatureControl(int numerator, int denominator, int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX() + (68 * dpiScale), position.y - tickToPosition(tick) + visualOffset};
		return eventControl(getTimelineEndX(), pos, timeColor, IO::formatString("%d/%d", numerator, denominator).c_str(), enabled);
	}

	bool ScoreEditorTimeline::skillControl(const SkillTrigger& skill)
	{
		return skillControl(skill.tick, !playing);
	}

	bool ScoreEditorTimeline::skillControl(int tick, bool enabled)
	{
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX() - (50 * dpiScale), position.y - tickToPosition(tick) + visualOffset};
		return eventControl(getTimelineStartX(), pos, skillColor, getString("skill"), enabled);
	}

	bool ScoreEditorTimeline::feverControl(const Fever& fever)
	{
		return feverControl(fever.startTick, true, !playing) || feverControl(fever.endTick, false, !playing);
	}

	bool ScoreEditorTimeline::feverControl(int tick, bool start, bool enabled)
	{
		if (tick < 0)
			return false;

		std::string txt = "FEVER";
		txt.append(start ? ICON_FA_CARET_UP : ICON_FA_CARET_DOWN);

		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX() - (105 * dpiScale), position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(), pos, feverColor, txt.c_str(), enabled);
	}

	bool ScoreEditorTimeline::hiSpeedControl(const HiSpeedChange& hiSpeed)
	{
		return hiSpeedControl(hiSpeed.tick, hiSpeed.speed);
	}

	bool ScoreEditorTimeline::hiSpeedControl(int tick, float speed)
	{
		std::string txt = IO::formatString("%.2fx", speed);
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX() + (108 * dpiScale), position.y - tickToPosition(tick) + visualOffset};
		return eventControl(getTimelineEndX(), pos, speedColor, txt.c_str(), true);
	}

	void ScoreEditorTimeline::eventEditor(ScoreContext& context)
	{
		ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
		if (ImGui::BeginPopup("edit_event"))
		{
			std::string editLabel{"edit_"};
			editLabel.append(eventTypes[(int)eventEdit.type]);
			ImGui::Text(getString(editLabel));
			ImGui::Separator();

			if (eventEdit.type == EventType::Bpm)
			{
				UI::beginPropertyColumns();

				Tempo& tempo = context.score.tempoChanges[eventEdit.editIndex];
				UI::addFloatProperty(getString("bpm"), eventEdit.editBpm, "%g");
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					tempo.bpm = std::clamp(eventEdit.editBpm, MIN_BPM, MAX_BPM);

					context.pushHistory("Change tempo", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first tempo change
				if (tempo.tick != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.tempoChanges.erase(context.score.tempoChanges.begin() + eventEdit.editIndex);
						context.pushHistory("Remove tempo change", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::TimeSignature)
			{
				UI::beginPropertyColumns();
				if (UI::timeSignatureSelect(eventEdit.editTimeSignatureNumerator, eventEdit.editTimeSignatureDenominator))
				{
					Score prev = context.score;
					TimeSignature& ts = context.score.timeSignatures[eventEdit.editIndex];
					ts.numerator = std::clamp(abs(eventEdit.editTimeSignatureNumerator), MIN_TIME_SIGN, MAX_TIME_SIGN);
					ts.denominator = std::clamp(abs(eventEdit.editTimeSignatureDenominator), MIN_TIME_SIGN, MAX_TIME_SIGN);

					context.pushHistory("Change time signature", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first time signature
				if (eventEdit.editIndex != 0)
				{
					ImGui::Separator();
					if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
					{
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.timeSignatures.erase(eventEdit.editIndex);
						context.pushHistory("Remove time signature", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::HiSpeed)
			{
				UI::beginPropertyColumns();
				UI::addFloatProperty(getString("hi_speed_speed"), eventEdit.editHiSpeed, "%g");
				HiSpeedChange& hiSpeed = context.score.hiSpeedChanges[eventEdit.editIndex];
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					Score prev = context.score;
					hiSpeed.speed = eventEdit.editHiSpeed;

					context.pushHistory("Change hi-speed", prev, context.score);
				}
				UI::endPropertyColumns();

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2)))
				{
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.hiSpeedChanges.erase(context.score.hiSpeedChanges.begin() + eventEdit.editIndex);
					context.pushHistory("Remove hi-speed change", prev, context.score);
				}
			}
			ImGui::EndPopup();
		}
	}

	bool ScoreEditorTimeline::isMouseInHoldPath(const Note& n1, const Note& n2, EaseType ease, float x, float y)
	{
		float xStart1 = laneToPosition(n1.lane);
		float xStart2 = laneToPosition(n1.lane + n1.width);
		float xEnd1 = laneToPosition(n2.lane);
		float xEnd2 = laneToPosition(n2.lane + n2.width);
		float y1 = getNoteYPosFromTick(n1.tick);
		float y2 = getNoteYPosFromTick(n2.tick);

		if (!isWithinRange(y, y1, y2))
			return false;

		float percent = (y - y1) / (y2 - y1);
		float iPercent = ease == EaseType::Linear ? percent : ease == EaseType::EaseIn ? easeIn(percent) : easeOut(percent);
		float x1 = lerp(xStart1, xEnd1, iPercent);
		float x2 = lerp(xStart2, xEnd2, iPercent);

		return isWithinRange(x, std::min(x1, x2), std::max(x1, x2));
	}

	void ScoreEditorTimeline::previousTick(ScoreContext& context)
	{
		if (playing || context.currentTick == 0)
			return;

		context.currentTick = std::max(roundTickDown(context.currentTick, division) - (TICKS_PER_BEAT / (division / 4)), 0);
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Down);
	}

	void ScoreEditorTimeline::nextTick(ScoreContext& context)
	{
		if (playing)
			return;

		context.currentTick = roundTickDown(context.currentTick, division) + (TICKS_PER_BEAT / (division / 4));
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Up);
	}

	int ScoreEditorTimeline::roundTickDown(int tick, int division)
	{
		return std::max(tick - (tick % (TICKS_PER_BEAT / (division / 4))), 0);
	}

	void ScoreEditorTimeline::insertNote(ScoreContext& context, EditArgs& edit, bool critical)
	{
		Score prev = context.score;

		Note newNote = inputNotes.tap;
		newNote.ID = nextID++;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert note", prev, context.score);
	}

	void ScoreEditorTimeline::insertHold(ScoreContext& context, EditArgs& edit)
	{
		Score prev = context.score;

		Note holdStart = inputNotes.holdStart;
		holdStart.ID = nextID++;

		Note holdEnd = inputNotes.holdEnd;
		holdEnd.ID = nextID++;
		holdEnd.parentID = holdStart.ID;

		if (holdStart.tick == holdEnd.tick)
		{
			const TimeSignature& t = context.score.timeSignatures.at(
				findTimeSignature(
					accumulateMeasures(
						holdStart.tick, TICKS_PER_BEAT, context.score.timeSignatures
					), context.score.timeSignatures
				)
			);

			holdEnd.tick += (beatsPerMeasure(t) * TICKS_PER_BEAT) / (((float)division / 4) * t.numerator);
		}
		else if (holdStart.tick > holdEnd.tick)
		{
			std::swap(holdStart.tick, holdEnd.tick);
			std::swap(holdStart.width, holdEnd.width);
			std::swap(holdStart.lane, holdEnd.lane);
		}

		context.score.notes[holdStart.ID] = holdStart;
		context.score.notes[holdEnd.ID] = holdEnd;
		context.score.holdNotes[holdStart.ID] = { {holdStart.ID, HoldStepType::Normal, edit.easeType}, {}, holdEnd.ID };
		context.pushHistory("Insert hold", prev, context.score);
	}

	void ScoreEditorTimeline::insertHoldStep(ScoreContext& context, EditArgs& edit, int holdId)
	{
		// make sure the hold data exists
		if (context.score.holdNotes.find(holdId) == context.score.holdNotes.end())
			return;

		// make sure the parent hold note exists
		if (context.score.notes.find(holdId) == context.score.notes.end())
			return;

		Score prev = context.score;

		HoldNote& hold = context.score.holdNotes[holdId];
		Note holdStart = context.score.notes[holdId];

		Note holdStep = inputNotes.holdStep;
		holdStep.ID = nextID++;
		holdStep.critical = holdStart.critical;
		holdStep.parentID = holdStart.ID;

		context.score.notes[holdStep.ID] = holdStep;

		hold.steps.push_back({ holdStep.ID, edit.stepType, edit.easeType });

		// sort steps in-case the step is inserted before/after existing steps
		sortHoldSteps(context.score, hold);
		context.pushHistory("Insert hold step", prev, context.score);
	}

	void ScoreEditorTimeline::debug()
	{
		ImGui::Text("Viewport position: (%f, %f)", position.x, position.y);
		ImGui::Text("Viewport size: (%f, %f)", size.x, size.y);

		ImGui::Text("Mouse position: (%f, %f)", mousePos.x, mousePos.y);

		ImGui::Text("Minimum offset: %f", minOffset);
		ImGui::Text("Current offset: %f", offset);
		ImGui::Text("Maximum offset: %f", maxOffset);

		ImGui::Text("Mouse in timeline: %s", boolToString(mouseInTimeline));

		ImGui::Text("Hover lane: %d", hoverLane);
		ImGui::Text("Hover tick: %d", hoverTick);

		ImGui::Text("Last selected tick: %d", lastSelectedTick);

		ImGui::InputInt("Note Cutoff X", &noteCutoffX);
		ImGui::InputInt("Note Slice Width", &noteSliceWidth);
		ImGui::InputInt("Note Offset X", &noteOffsetX);
		ImGui::InputInt("Note Slice Size", &notesSliceSize);
	}

	ScoreEditorTimeline::ScoreEditorTimeline()
	{
		framebuffer = std::make_unique<Framebuffer>(1920, 1080);

		background.load(config.backgroundImage.empty() ? (Application::getAppDir() + "res/textures/default.png") : config.backgroundImage);
		background.setBrightness(0.67);
	}

	void ScoreEditorTimeline::togglePlaying(ScoreContext& context)
	{
		playing ^= true;
		if (playing)
		{
			playStartTime = time;
			context.audio.seekBGM(time);
			context.audio.reSync();
			context.audio.playBGM(time);
		}
		else
		{
			if (config.returnToLastSelectedTickOnPause)
			{
				context.currentTick = lastSelectedTick;
				offset = std::max(minOffset, tickToPosition(context.currentTick) + (size.y * (1.0f - config.cursorPositionThreshold)));
			}

			context.audio.stopSounds(false);
			context.audio.stopBGM();
		}
	}

	void ScoreEditorTimeline::stop(ScoreContext& context)
	{
		playing = false;
		time = lastSelectedTick = context.currentTick = 0;

		context.audio.stopSounds(false);
		context.audio.stopBGM();
		
		offset = std::max(minOffset, tickToPosition(context.currentTick) + (size.y * (1.0f - config.cursorPositionThreshold)));
	}

	void ScoreEditorTimeline::updateNoteSE(ScoreContext& context)
	{
		if (!playing)
			return;

		tickSEMap.clear();
		for (const auto& it : context.score.notes)
		{
			const Note& note = it.second;
			float noteTime = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float notePlayTime = noteTime - playStartTime;
			float offsetNoteTime = noteTime - audioLookAhead;

			if (offsetNoteTime >= timeLastFrame && offsetNoteTime < time)
			{
				std::string se = getNoteSE(note, context.score);
				std::string key = std::to_string(note.tick) + "-" + se;
				if (se.size());
				{
					if (tickSEMap.find(key) == tickSEMap.end())
					{
						context.audio.playSound(se.c_str(), notePlayTime - audioOffsetCorrection, -1);
						tickSEMap[key] = note.tick;
					}
				}

				if (note.getType() == NoteType::Hold)
				{
					float endTime = accumulateDuration(context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick, TICKS_PER_BEAT, context.score.tempoChanges);
					context.audio.playSound(note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT, notePlayTime - audioOffsetCorrection, endTime - playStartTime);
				}
			}
			else if (time == playStartTime)
			{
				// playback just started
				if (noteTime >= time && offsetNoteTime < time)
				{
					std::string se = getNoteSE(note, context.score);
					std::string key = std::to_string(note.tick) + "-" + se;
					if (se.size())
					{
						if (tickSEMap.find(key) == tickSEMap.end())
						{
							context.audio.playSound(se.c_str(), notePlayTime, -1);
							tickSEMap[key] = note.tick;
						}
					}
				}

				// playback started mid-hold
				if (note.getType() == NoteType::Hold)
				{
					int endTick = context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
					float endTime = accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);

					if ((noteTime - time) <= audioLookAhead && endTime > time)
						context.audio.playSound(note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT, std::max(0.0f, notePlayTime), endTime - playStartTime);
				}
			}
		}
	}
}
