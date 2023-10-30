#include "ScoreEditorTimeline.h"
#include "Application.h"
#include "ApplicationConfiguration.h"
#include "Colors.h"
#include "Constants.h"
#include "ResourceManager.h"
#include "Tempo.h"
#include "UI.h"
#include "Utilities.h"
#include <algorithm>
#include <string>

namespace MikuMikuWorld {
	bool eventControl(float xPos, Vector2 pos, ImU32 color, const char* txt,
		bool enabled) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return false;

		pos.x = floorf(pos.x);
		pos.y = floorf(pos.y);

		ImGui::PushID(pos.y);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {1, 0});
		bool activated = UI::coloredButton(txt, { pos.x, pos.y - ImGui::GetFrameHeightWithSpacing() }, { -1, -1 }, color, enabled);
		ImGui::PopStyleVar();
		ImGui::PopID();
		drawList->AddLine({ xPos, pos.y }, { pos.x + ImGui::GetItemRectSize().x, pos.y },
			color, primaryLineThickness);

		return activated;
	}

	float ScoreEditorTimeline::getNoteYPosFromTick(int tick) const {
		return position.y + tickToPosition(tick) - visualOffset + size.y;
	}

	int ScoreEditorTimeline::positionToTick(float pos) const {
		return roundf(pos / (unitHeight * zoom));
	}

	float ScoreEditorTimeline::tickToPosition(int tick) const {
		return tick * unitHeight * zoom;
	}

	int ScoreEditorTimeline::positionToLane(float pos) const {
		return floor((pos - laneOffset) / laneWidth);
	}

	float ScoreEditorTimeline::laneToPosition(float lane) const {
		return laneOffset + (lane * laneWidth);
	}

	bool ScoreEditorTimeline::isNoteVisible(const Note& note,
		int offsetTicks) const {
		const float y = getNoteYPosFromTick(note.tick + offsetTicks);
		return y >= 0 && y <= size.y + position.y + 100;
	}

	void ScoreEditorTimeline::setZoom(float value) {
		int tick = positionToTick(offset - size.y);
		float x1 = position.y - tickToPosition(tick) + offset;

		zoom = std::clamp(value, minZoom, maxZoom);

		// Prevent jittery movement when zooming
		float x2 = position.y - tickToPosition(tick) + offset;
		visualOffset = offset = std::max(offset + x1 - x2, minOffset);
	}

	int ScoreEditorTimeline::snapTickFromPos(float posY) const
	{
		return snapTick(positionToTick(posY), division);
	}

	int ScoreEditorTimeline::laneFromCenterPosition(const Score& score, int lane, int width)
	{
		return std::clamp(lane - (width / 2), MIN_LANE - score.metadata.laneExtension, MAX_LANE - width + 1 + score.metadata.laneExtension);
	}

	void ScoreEditorTimeline::focusCursor(ScoreContext& context,
		Direction direction) {
		float cursorY = tickToPosition(context.currentTick);
		if (direction == Direction::Down) {
			float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
			if (cursorY <= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
		else {
			float timelineOffset = size.y * config.cursorPositionThreshold;
			if (cursorY >= offset - timelineOffset)
				offset = cursorY + timelineOffset;
		}
	}

	void ScoreEditorTimeline::updateScrollbar() {
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
		float paddingY = 30.0f;
		ImVec2 windowEndTop =
			ImGui::GetWindowPos() +
			ImVec2{ ImGui::GetWindowSize().x - scrollbarWidth - 4, paddingY };
		ImVec2 windowEndBottom =
			windowEndTop +
			ImVec2{ scrollbarWidth + 2, ImGui::GetWindowSize().y - (paddingY * 1.3f) -
																		 UI::toolbarBtnSize.y - 5 };

		// calculate handle height
		float heightRatio = size.y / ((maxOffset - minOffset) * zoom);
		float handleHeight = std::max(
			20.0f, ((windowEndBottom.y - windowEndTop.y) * heightRatio) + 30.0f);
		float scrollHeight = windowEndBottom.y - windowEndTop.y - handleHeight;

		// calculate handle position
		float currentOffset = offset - minOffset;
		float positionRatio =
			std::min(1.0f, currentOffset / ((maxOffset * zoom) - minOffset));
		float handlePosition =
			windowEndBottom.y - (scrollHeight * positionRatio) - handleHeight;

		// make handle slightly narrower than the background
		ImVec2 scrollHandleMin = ImVec2{ windowEndTop.x + 2, handlePosition };
		ImVec2 scrollHandleMax = ImVec2{ windowEndTop.x + scrollbarWidth - 2,
																		handlePosition + handleHeight };

		// handle "button"
		ImGuiCol handleColor = ImGuiCol_ScrollbarGrab;
		ImGui::SetCursorScreenPos(scrollHandleMin);
		ImGui::InvisibleButton("##scroll_handle",
			ImVec2{ scrollbarWidth, handleHeight });
		if (ImGui::IsItemHovered())
			handleColor = ImGuiCol_ScrollbarGrabHovered;

		if (ImGui::IsItemActivated())
			scrollStartY = ImGui::GetMousePos().y;

		if (ImGui::IsItemActive()) {
			handleColor = ImGuiCol_ScrollbarGrabActive;
			float dragDeltaY = scrollStartY - ImGui::GetMousePos().y;
			if (abs(dragDeltaY) > 0) {
				// convert handle position to timeline offset
				handlePosition -= dragDeltaY;
				positionRatio = std::min(
					1.0f, 1 - ((handlePosition - windowEndTop.y) / scrollHeight));
				float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

				offset = newOffset + minOffset;
				scrollStartY = ImGui::GetMousePos().y;
			}
		}

		if (!ImGui::IsItemActive() && positionRatio >= 0.92f)
			maxOffset += 2000;

		ImGui::SetCursorScreenPos(windowEndTop);
		ImGui::InvisibleButton("##scroll_background",
			ImVec2{ scrollbarWidth, scrollHeight + handleHeight },
			ImGuiButtonFlags_AllowItemOverlap);
		if (ImGui::IsItemActivated()) {
			float yPos = std::clamp(ImGui::GetMousePos().y, windowEndTop.y,
				windowEndBottom.y - handleHeight);

			// convert handle position to timeline offset
			positionRatio =
				std::clamp(1 - ((yPos - windowEndTop.y)) / scrollHeight, 0.0f, 1.0f);
			float newOffset = ((maxOffset * zoom) - minOffset) * positionRatio;

			offset = newOffset + minOffset;
		}

		ImU32 scrollBgColor = ImGui::ColorConvertFloat4ToU32(
			ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg));
		ImU32 scrollHandleColor =
			ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(handleColor));
		drawList->AddRectFilled(windowEndTop, windowEndBottom, scrollBgColor, 0);
		drawList->AddRectFilled(scrollHandleMin, scrollHandleMax, scrollHandleColor,
			ImGui::GetStyle().ScrollbarRounding,
			ImDrawFlags_RoundCornersAll);
	}

	void ScoreEditorTimeline::calculateMaxOffsetFromScore(const Score& score) {
		int maxTick = 0;
		for (const auto& [id, note] : score.notes)
			maxTick = std::max(maxTick, note.tick);

		// Current offset maybe greater than calculated offset from score
		maxOffset = std::max(offset / zoom, (maxTick * unitHeight) + 1000);
	}

	void ScoreEditorTimeline::updateScrollingPosition() {
		if (config.useSmoothScrolling) {
			float scrollAmount = offset - visualOffset;
			float remainingScroll = abs(scrollAmount);
			float delta = scrollAmount / (config.smoothScrollingTime /
				(ImGui::GetIO().DeltaTime * 1000));

			visualOffset += std::min(remainingScroll, delta);
			remainingScroll = std::max(0.0f, remainingScroll - abs(delta));
		}
		else {
			visualOffset = offset;
		}
	}

	void ScoreEditorTimeline::contextMenu(ScoreContext& context)
	{
		if (ImGui::BeginPopupContextWindow(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
		{
			if (ImGui::MenuItem(getString("delete"), ToShortcutString(config.input.deleteSelection), false, !context.selectedNotes.empty()))
				context.deleteSelection();

			ImGui::Separator();
			if (ImGui::MenuItem(getString("cut"), ToShortcutString(config.input.cutSelection), false, !context.selectedNotes.empty()))
				context.cutSelection();

			if (ImGui::MenuItem(getString("copy"), ToShortcutString(config.input.copySelection), false, !context.selectedNotes.empty()))
				context.copySelection();

			if (ImGui::MenuItem(getString("paste"),
				ToShortcutString(config.input.paste)))
				context.paste(false);

			if (ImGui::MenuItem(getString("flip_paste"),
				ToShortcutString(config.input.flipPaste)))
				context.paste(true);

			if (ImGui::MenuItem(getString("flip"), ToShortcutString(config.input.flip), false, !context.selectedNotes.empty()))
				context.flipSelection();

			ImGui::Separator();
			if (ImGui::BeginMenu(getString("ease_type"), context.selectionHasEase()))
			{
				for (int i = 0; i < arrayLength(easeTypes); ++i)
					if (ImGui::MenuItem(getString(easeTypes[i]))) context.setEase((EaseType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("step_type"), context.selectionHasStep()))
			{
				for (int i = 0; i < arrayLength(stepTypes); ++i)
					if (ImGui::MenuItem(getString(stepTypes[i]))) context.setStep((HoldStepType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("flick_type"), context.selectionHasFlickable()))
			{
				for (int i = 0; i < arrayLength(flickTypes); ++i)
					if (ImGui::MenuItem(getString(flickTypes[i]))) context.setFlick((FlickType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("hold_type"),
				context.selectionCanChangeHoldType())) {
				/*
				 * We we won't show the option to change holds to guides and vice versa
				 * at least for now since it will complicate changing hold types
				 */
				for (int i = 0; i < arrayLength(holdTypes) - 1; i++)
					if (ImGui::MenuItem(getString(holdTypes[i])))
						context.setHoldType((HoldNoteType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("fade_type"),
				context.selectionCanChangeFadeType())) {
				for (int i = 0; i < arrayLength(fadeTypes); ++i)
					if (ImGui::MenuItem(getString(fadeTypes[i])))
						context.setFadeType((FadeType)i);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("guide_color"),
				context.selectionCanChangeFadeType())) {
				for (int i = 0; i < arrayLength(guideColors); ++i) {
					char str[32];
					sprintf_s(str, "guide_%s", guideColors[i]);
					if (ImGui::MenuItem(getString(str)))
						context.setGuideColor((GuideColor)i);
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu(getString("layer"),
				context.selectedNotes.size() > 0)) {
				for (int i = 0; i < context.score.layers.size(); ++i)
					if (ImGui::MenuItem(context.score.layers[i].name.c_str()))
						context.setLayer(i);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::MenuItem(getString("shrink_up"), NULL, false, !context.selectedNotes.empty()))
				context.shrinkSelection(Direction::Up);

			if (ImGui::MenuItem(getString("shrink_down"), NULL, false, !context.selectedNotes.empty()))
				context.shrinkSelection(Direction::Down);

			ImGui::Separator();
			if (ImGui::MenuItem(getString("connect_holds"), NULL, false,
				context.selectionCanConnect()))
				context.connectHoldsInSelection();

			if (ImGui::MenuItem(getString("split_hold"), NULL, false,
				context.selectionHasStep() &&
				context.selectedNotes.size() == 1))
				context.splitHoldInSelection();

			ImGui::EndPopup();
		}
	}

	void ScoreEditorTimeline::update(ScoreContext& context, EditArgs& edit,
		Renderer* renderer) {
		prevSize = size;
		prevPos = position;

		// Make space for the scrollbar and the status bar
		size = ImGui::GetContentRegionAvail() - ImVec2{ ImGui::GetStyle().ScrollbarSize, UI::toolbarBtnSize.y };
		position = ImGui::GetCursorScreenPos();
		boundaries = ImRect(position, position + size);
		mouseInTimeline = ImGui::IsMouseHoveringRect(position, position + size);

		laneOffset = (size.x * 0.5f) - ((NUM_LANES * laneWidth) * 0.5f);
		minOffset = size.y - 50;

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(boundaries.Min, boundaries.Max, true);
		drawList->AddRectFilled(boundaries.Min, boundaries.Max, 0xff202020);

		if (background.isDirty()) {
			background.resize({ size.x, size.y });
			background.process(renderer);
		}
		else if (prevSize.x != size.x || prevSize.y != size.y) {
			background.resize({ size.x, size.y });
		}

		if (config.drawBackground)
		{
			const float bgWidth = static_cast<float>(background.getWidth());
			const float bgHeight = static_cast<float>(background.getHeight());
			ImVec2 bgPos{ position.x - (abs(bgWidth - size.x) / 2.0f), position.y - (abs(bgHeight - size.y) / 2.0f) };
			drawList->AddImage((ImTextureID)background.getTextureID(), bgPos, bgPos + ImVec2{ bgWidth, bgHeight });
		}

		// Remember whether the last mouse click was in the timeline or not
		static bool clickedOnTimeline = false;
		if (ImGui::IsMouseClicked(0))
			clickedOnTimeline = mouseInTimeline;

		const bool pasting = context.pasteData.pasting;
		const ImGuiIO& io = ImGui::GetIO();
		// Get mouse position relative to timeline
		mousePos = io.MousePos - position;
		mousePos.y -= offset;
		if (mouseInTimeline && !UI::isAnyPopupOpen()) {
			if (io.KeyCtrl) {
				setZoom(zoom + (io.MouseWheel * 0.1f));
			}
			else {
				float scrollAmount = io.MouseWheel * scrollUnit;
				offset += scrollAmount * (io.KeyShift ? config.scrollSpeedShift
					: config.scrollSpeedNormal);
			}

			if (!isHoveringNote && !isHoldingNote && !insertingHold && !pasting && currentMode == TimelineMode::Select)
			{
				// Clicked inside timeline, the current mouse position is the first drag point
				if (ImGui::IsMouseClicked(0))
				{
					dragStart = mousePos;
					if (!io.KeyCtrl && !io.KeyAlt &&
						!ImGui::IsPopupOpen(IMGUI_TITLE(ICON_FA_MUSIC, "notes_timeline")))
						context.selectedNotes.clear();
				}

				// Clicked and dragging inside the timeline
				if (clickedOnTimeline && ImGui::IsMouseDown(0) && ImGui::IsMouseDragPastThreshold(0, 10.0f))
					dragging = true;
			}
		}

		offset = std::max(offset, minOffset);
		updateScrollingPosition();

		// Selection rectangle
		// Draw selection rectangle after notes are rendered
		if (dragging && ImGui::IsMouseReleased(0) && !pasting)
		{
			// Calculate drag selection
			float left = std::min(dragStart.x, mousePos.x);
			float right = std::max(dragStart.x, mousePos.x);
			float top = std::min(dragStart.y, mousePos.y);
			float bottom = std::max(dragStart.y, mousePos.y);

			if (!io.KeyAlt && !io.KeyCtrl)
				context.selectedNotes.clear();

			float yThreshold = (notesHeight * 0.5f) + 2.0f;
			for (const auto& [id, note] : context.score.notes) {
				if (note.layer != context.selectedLayer && !context.showAllLayers)
					continue;
				float x1 = laneToPosition(note.lane);
				float x2 = laneToPosition(note.lane + note.width);
				float y = -tickToPosition(note.tick);

				if (right > x1 && left < x2 &&
					isWithinRange(y, top - yThreshold, bottom + yThreshold)) {
					if (io.KeyAlt)
						context.selectedNotes.erase(id);
					else
						context.selectedNotes.insert(id);
				}
			}

			dragging = false;
		}

		const float x1 = getTimelineStartX();
		const float x2 = getTimelineEndX();
		const float exX1 = getTimelineStartX(context.score);
		const float exX2 = getTimelineEndX(context.score);

		// Draw solid background color
		drawList->AddRectFilled(
			{ exX1, position.y }, { x1, position.y + size.y },
			Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255),
				0x1c, 0x1a, 0x0f));
		drawList->AddRectFilled(
			{ x1, position.y }, { x2, position.y + size.y },
			Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255),
				0x1c, 0x1a, 0x1f));
		drawList->AddRectFilled(
			{ x2, position.y }, { exX2, position.y + size.y },
			Color::abgrToInt(std::clamp((int)(config.laneOpacity * 255), 0, 255),
				0x1c, 0x1a, 0x0f));

		if (config.drawWaveform)
			drawWaveform(context);

		// Draw lanes
		for (int l = 0; l <= NUM_LANES; ++l)
		{
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y), boldLane ? divColor1 : divColor2, boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		// Draw measures
		int firstTick = std::max(0, positionToTick(visualOffset - size.y));
		int lastTick = positionToTick(visualOffset);
		int measure = accumulateMeasures(firstTick, TICKS_PER_BEAT,
			context.score.timeSignatures);
		firstTick =
			measureToTicks(measure, TICKS_PER_BEAT, context.score.timeSignatures);

		int tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		int ticksPerMeasure =
			beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;
		int beatTicks =
			ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;
		int subdivision = TICKS_PER_BEAT / (division / 4);

		// Snap to the sub-division before the current measure to prevent the lines from jumping around
		for (int tick = firstTick - (firstTick % subdivision); tick <= lastTick; tick += subdivision)
		{
			const int y = position.y - tickToPosition(tick) + visualOffset;
			int currentMeasure = accumulateMeasures(tick, TICKS_PER_BEAT, context.score.timeSignatures);

			// Time signature changes on current measure
			if (context.score.timeSignatures.find(currentMeasure) != context.score.timeSignatures.end() && currentMeasure != tsIndex)
			{
				tsIndex = currentMeasure;
				ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) *
					TICKS_PER_BEAT;
				beatTicks =
					ticksPerMeasure / context.score.timeSignatures[tsIndex].numerator;

				// snap to sub-division again on time signature change
				tick = measureToTicks(currentMeasure, TICKS_PER_BEAT,
					context.score.timeSignatures);
				tick -= tick % subdivision;
			}

			// determine whether the tick is a beat relative to its measure's tick
			int measureTicks = measureToTicks(currentMeasure, TICKS_PER_BEAT,
				context.score.timeSignatures);

			ImU32 color;
			ImU32 exColor;
			float thickness;
			if (!((tick - measureTicks) % beatTicks)) {
				color = measureColor;
				exColor = exMeasureColor;
				thickness = primaryLineThickness;
			}
			else if (division >= 192) {
				continue;
			}
			else {
				color = divColor2;
				exColor = exDivColor2;
				thickness = secondaryLineThickness;
			}

			drawList->AddLine(ImVec2(exX1, y), ImVec2(x1, y), exColor, thickness);
			drawList->AddLine(ImVec2(x1, y), ImVec2(x2, y), color, thickness);
			drawList->AddLine(ImVec2(x2, y), ImVec2(exX2, y), exColor, thickness);
		}

		tsIndex = findTimeSignature(measure, context.score.timeSignatures);
		ticksPerMeasure =
			beatsPerMeasure(context.score.timeSignatures[tsIndex]) * TICKS_PER_BEAT;

		// Overdraw one measure to make sure the measure string is always visible
		for (int tick = firstTick; tick < lastTick + ticksPerMeasure; tick += ticksPerMeasure)
		{
			if (context.score.timeSignatures.find(measure) != context.score.timeSignatures.end())
			{
				tsIndex = measure;
				ticksPerMeasure = beatsPerMeasure(context.score.timeSignatures[tsIndex]) *
					TICKS_PER_BEAT;
			}

			std::string measureStr = std::to_string(measure);
			const float txtPos = x1 - MEASURE_WIDTH - (ImGui::CalcTextSize(measureStr.c_str()).x * 0.5f);
			const int y = position.y - tickToPosition(tick) + visualOffset;

			drawList->AddLine(ImVec2(exX1 - MEASURE_WIDTH, y),
				ImVec2(exX2 + MEASURE_WIDTH, y), measureColor,
				primaryLineThickness);
			drawShadedText(drawList, ImVec2(txtPos, y), 26, measureTxtColor,
				measureStr.c_str());

			++measure;
		}

		// draw lanes
		for (int l = -context.score.metadata.laneExtension;
			l <= NUM_LANES + context.score.metadata.laneExtension; ++l) {
			const int x = position.x + laneToPosition(l);
			const bool boldLane = !(l & 1);
			const bool outOfBounds = l < 0 || l > NUM_LANES;
			drawList->AddLine(ImVec2(x, position.y), ImVec2(x, position.y + size.y),
				outOfBounds ? boldLane ? exDivColor1 : exDivColor2
				: boldLane ? divColor1
				: divColor2,
				boldLane ? primaryLineThickness : secondaryLineThickness);
		}

		hoverTick = snapTickFromPos(-mousePos.y);
		hoverLane = positionToLane(mousePos.x);
		isHoveringNote = false;

		// Draw cursor behind notes
		const float y = position.y - tickToPosition(context.currentTick) + visualOffset;
		const int triPtOffset = 6;
		const int triXPos = exX1 - (triPtOffset * 2);
		drawList->AddTriangleFilled(
			ImVec2(triXPos, y - triPtOffset), ImVec2(triXPos, y + triPtOffset),
			ImVec2(triXPos + (triPtOffset * 2), y), cursorColor);
		drawList->AddLine(ImVec2(exX1, y), ImVec2(exX2, y), cursorColor,
			primaryLineThickness + 1.0f);

		contextMenu(context);

		// Update hi-speed changes
		for (int index = 0; index < context.score.hiSpeedChanges.size(); ++index)
		{
			HiSpeedChange& hiSpeed = context.score.hiSpeedChanges[index];
			if (hiSpeedControl(context, hiSpeed)) {
				eventEdit.editIndex = index;
				eventEdit.editHiSpeed = hiSpeed.speed;
				eventEdit.type = EventType::HiSpeed;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update time signature changes
		for (auto& [measure, ts] : context.score.timeSignatures)
		{
			if (timeSignatureControl(context.score, ts.numerator, ts.denominator, measureToTicks(ts.measure, TICKS_PER_BEAT, context.score.timeSignatures), !playing))
			{
				eventEdit.editIndex = measure;
				eventEdit.editTimeSignatureNumerator = ts.numerator;
				eventEdit.editTimeSignatureDenominator = ts.denominator;
				eventEdit.type = EventType::TimeSignature;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update bpm changes
		for (int index = 0; index < context.score.tempoChanges.size(); ++index)
		{
			Tempo& tempo = context.score.tempoChanges[index];
			if (bpmControl(context.score, tempo)) {
				eventEdit.editIndex = index;
				eventEdit.editBpm = tempo.bpm;
				eventEdit.type = EventType::Bpm;
				ImGui::OpenPopup("edit_event");
			}
		}

		// Update song boundaries
		if (context.audio.isMusicInitialized())
		{
			int startTick = accumulateTicks(context.workingData.musicOffset / 1000, TICKS_PER_BEAT, context.score.tempoChanges);
			int endTick = accumulateTicks(context.audio.getMusicEndTime(), TICKS_PER_BEAT, context.score.tempoChanges);

			float x = getTimelineEndX(context.score);
			float y1 = position.y - tickToPosition(startTick) + visualOffset;
			float y2 = position.y - tickToPosition(endTick) + visualOffset;

			drawList->AddTriangleFilled({ x, y1 }, { x + 10, y1 }, { x + 10, y1 - 10 },
				0xFFCCCCCC);
			drawList->AddTriangleFilled({ x, y2 }, { x + 10, y2 }, { x + 10, y2 + 10 },
				0xFFCCCCCC);
		}

		feverControl(context.score, context.score.fever);

		// Update skill triggers
		for (const auto& skill : context.score.skills)
			skillControl(context.score, skill);

		eventEditor(context);
		updateNotes(context, edit, renderer);

		// Update cursor tick after determining whether a note is hovered
		// The cursor tick should not change if a note is hovered
		if (ImGui::IsMouseClicked(0) && !isHoveringNote && mouseInTimeline && !playing && !pasting &&
			!UI::isAnyPopupOpen() && currentMode == TimelineMode::Select && ImGui::IsWindowFocused())
		{
			context.currentTick = hoverTick;
			lastSelectedTick = context.currentTick;
		}

		// Selection boxes
		for (int id : context.selectedNotes)
		{
			const Note& note = context.score.notes.at(id);
			if (!isNoteVisible(note, 0))
				continue;

			float x = position.x;
			float y = position.y - tickToPosition(note.tick) + visualOffset;

			ImVec2 p1{ x + laneToPosition(note.lane) - 3, y - (notesHeight * 0.5f) };
			ImVec2 p2{ x + laneToPosition(note.lane + note.width) + 3,
								y + (notesHeight * 0.5f) };

			drawList->AddRectFilled(p1, p2, 0x20f4f4f4, 2.0f,
				ImDrawFlags_RoundCornersAll);
			drawList->AddRect(p1, p2, 0xcccccccc, 2.0f, ImDrawFlags_RoundCornersAll,
				2.0f);
		}

		if (dragging && !pasting) {
			float startX = std::min(position.x + dragStart.x, position.x + mousePos.x);
			float endX = std::max(position.x + dragStart.x, position.x + mousePos.x);
			float startY = std::min(position.y + dragStart.y, position.y + mousePos.y) +
				visualOffset;
			float endY = std::max(position.y + dragStart.y, position.y + mousePos.y) +
				visualOffset;
			ImVec2 start{ startX, startY };
			ImVec2 end{ endX, endY };

			drawList->AddRectFilled(start, end, selectionColor1);
			drawList->AddRect(start, end, 0xbbcccccc, 0.2f, ImDrawFlags_RoundCornersAll,
				1.0f);

			ImVec2 iconPos = ImVec2(position + dragStart);
			iconPos.y += visualOffset;
			if (io.KeyCtrl) {
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd,
					ICON_FA_PLUS_CIRCLE);
			}
			else if (io.KeyAlt) {
				drawList->AddText(ImGui::GetFont(), 12, iconPos, 0xdddddddd,
					ICON_FA_MINUS_CIRCLE);
			}
		}

		drawList->PopClipRect();

		int currentMeasure = accumulateMeasures(context.currentTick, TICKS_PER_BEAT, context.score.timeSignatures);
		const TimeSignature& ts = context.score.timeSignatures[findTimeSignature(currentMeasure, context.score.timeSignatures)];
		const Tempo& tempo = getTempoAt(context.currentTick, context.score.tempoChanges);

		int hiSpeed = findHighSpeedChange(context.currentTick, context.score.hiSpeedChanges, context.selectedLayer);
		float speed = (hiSpeed == -1 ? 1.0f : context.score.hiSpeedChanges[hiSpeed].speed);

		std::string rhythmString = IO::formatString(
			"  %02d:%02d:%02d  |  %d/%d  |  %g BPM  |  %gx",
			(int)time / 60, (int)time % 60, (int)((time - (int)time) * 100),
			ts.numerator, ts.denominator,
			tempo.bpm,
			speed
		);

		// Status bar: playback controls, division, zoom, current time and rhythm
		ImGui::SetCursorPos(ImVec2{ ImGui::GetStyle().WindowPadding.x, size.y + UI::toolbarBtnSize.y + ImGui::GetStyle().WindowPadding.y });
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });

		ImGui::PushButtonRepeat(true);
		if (UI::toolbarButton(ICON_FA_BACKWARD, "", NULL,
			!playing && context.currentTick > 0))
			previousTick(context);
		ImGui::PopButtonRepeat();

		ImGui::SameLine();
		if (UI::toolbarButton(ICON_FA_STOP, "", NULL))
			stop(context);

		ImGui::SameLine();
		if (UI::toolbarButton(playing ? ICON_FA_PAUSE : ICON_FA_PLAY, "", NULL))
			setPlaying(context, !playing);

		ImGui::SameLine();
		ImGui::PushButtonRepeat(true);
		if (UI::toolbarButton(ICON_FA_FORWARD, "", NULL, !playing))
			nextTick(context);
		ImGui::PopButtonRepeat();

		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		UI::divisionSelect(getString("division"), division, divisions,
			sizeof(divisions) / sizeof(int));

		static int gotoMeasure = 0;
		bool activated = false;

		ImGui::SameLine();
		ImGui::SetNextItemWidth(50);
		ImGui::InputInt("##goto_measure", &gotoMeasure, 0, 0,
			ImGuiInputTextFlags_AutoSelectAll);
		activated |=
			ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter, false);
		UI::tooltip(getString("goto_measure"));

		ImGui::SameLine();
		activated |= UI::transparentButton(ICON_FA_ARROW_RIGHT, UI::btnSmall);

		if (activated) {
			gotoMeasure = std::clamp(gotoMeasure, 0, 999);
			context.currentTick = measureToTicks(gotoMeasure, TICKS_PER_BEAT,
				context.score.timeSignatures);
			offset = std::max(minOffset,
				tickToPosition(context.currentTick) +
				(size.y * (1.0f - config.cursorPositionThreshold)));
		}

		ImGui::SameLine();
		float _zoom = zoom;
		int controlWidth = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(rhythmString.c_str()).x - (UI::btnSmall.x * 3);
		if (UI::zoomControl("zoom", _zoom, minZoom, 10, std::clamp(controlWidth, 120, 320)))
			setZoom(_zoom);

		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		ImGui::Text(rhythmString.c_str());

		updateScrollbar();

		updateNoteSE(context);

		timeLastFrame = time;
		if (playing) {
			time += ImGui::GetIO().DeltaTime;
			context.currentTick =
				accumulateTicks(time, TICKS_PER_BEAT, context.score.tempoChanges);

			float cursorY = tickToPosition(context.currentTick);
			if (config.followCursorInPlayback) {
				float timelineOffset = size.y * (1.0f - config.cursorPositionThreshold);
				if (cursorY >= offset - timelineOffset)
					visualOffset = offset = cursorY + timelineOffset;
			}
			else if (cursorY > offset) {
				visualOffset = offset = cursorY + size.y;
			}
		}
		else {
			time = accumulateDuration(context.currentTick, TICKS_PER_BEAT,
				context.score.tempoChanges);
		}
	}

	void ScoreEditorTimeline::updateNotes(ScoreContext& context, EditArgs& edit,
		Renderer* renderer) {
		// directxmath dies
		if (size.y < 10 || size.x < 10)
			return;

		Shader* shader = ResourceManager::shaders[0];
		shader->use();
		shader->setMatrix4("projection",
			camera.getOffCenterOrthographicProjection(
				0, size.x, position.y, position.y + size.y));

		glEnable(GL_FRAMEBUFFER_SRGB);
		framebuffer->bind();
		framebuffer->clear();
		renderer->beginBatch();

		minNoteYDistance = INT_MAX;
		for (auto& [id, note] : context.score.notes) {
			if (!isNoteVisible(note))
				continue;
			if (note.getType() == NoteType::Tap) {
				updateNote(context, edit, note);
				drawNote(note, renderer, (context.showAllLayers || note.layer == context.selectedLayer) ? noteTint : otherLayerTint);
			}
			if (note.getType() == NoteType::Damage) {
				updateNote(context, edit, note);
				drawCcNote(note, renderer, (context.showAllLayers || note.layer == context.selectedLayer) ? noteTint : otherLayerTint);
			}
		}

		for (auto& [id, hold] : context.score.holdNotes) {
			Note& start = context.score.notes.at(hold.start.ID);
			Note& end = context.score.notes.at(hold.end);

			if (isNoteVisible(start))
				updateNote(context, edit, start);
			if (isNoteVisible(end))
				updateNote(context, edit, end);

			for (const auto& step : hold.steps) {
				Note& mid = context.score.notes.at(step.ID);
				if (isNoteVisible(mid))
					updateNote(context, edit, mid);
				if (skipUpdateAfterSortingSteps)
					break;
			}

			drawHoldNote(context.score.notes, hold, renderer, noteTint, context.showAllLayers ? -1 : context.selectedLayer);
		}
		skipUpdateAfterSortingSteps = false;

		renderer->endBatch();
		renderer->beginBatch();

		const bool pasting = context.pasteData.pasting;
		if (pasting && mouseInTimeline && !playing) {
			context.pasteData.offsetTicks = hoverTick;
			context.pasteData.offsetLane = hoverLane;
			previewPaste(context, renderer);
			if (ImGui::IsMouseClicked(0))
				context.confirmPaste();
			else if (ImGui::IsMouseClicked(1))
				context.cancelPaste();
		}

		if (mouseInTimeline && !isHoldingNote &&
			currentMode != TimelineMode::Select && !pasting && !playing &&
			!UI::isAnyPopupOpen()) {
			previewInput(context, edit, renderer);
			if (ImGui::IsMouseClicked(0) && hoverTick >= 0 && !isHoveringNote)
				executeInput(context, edit);

			if (insertingHold && !ImGui::IsMouseDown(0)) {
				insertHold(context, edit);
				insertingHold = false;
			}
		}
		else {
			insertingHold = false;
		}

		renderer->endBatch();

		glDisable(GL_FRAMEBUFFER_SRGB);
		glDisable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddImage((void*)framebuffer->getTexture(), position,
			position + size);

		// draw hold step outlines
		for (const auto& data : drawSteps)
			drawOutline(data, context.showAllLayers ? -1 : context.selectedLayer);

		drawSteps.clear();
	}

	void ScoreEditorTimeline::previewPaste(ScoreContext& context,
		Renderer* renderer) {
		context.pasteData.offsetLane = std::clamp(
			hoverLane - context.pasteData.midLane, context.pasteData.minLaneOffset,
			context.pasteData.maxLaneOffset);

		for (const auto notes : { context.pasteData.notes, context.pasteData.damages })
			for (const auto& [_, note] : notes)
				if (isNoteVisible(note, hoverTick)) {
					if (note.getType() == NoteType::Tap)
						drawNote(note, renderer, hoverTint, hoverTick,
							context.pasteData.offsetLane);
					else if (note.getType() == NoteType::Damage)
						drawCcNote(note, renderer, hoverTint, hoverTick,
							context.pasteData.offsetLane);
				}

		for (const auto& [_, hold] : context.pasteData.holds)
			drawHoldNote(context.pasteData.notes, hold, renderer, hoverTint, -1, hoverTick,
				context.pasteData.offsetLane);
	}

	void ScoreEditorTimeline::updateInputNotes(const Score& score, EditArgs& edit) {
		int lane = laneFromCenterPosition(score, hoverLane, edit.noteWidth);
		int width = edit.noteWidth;
		int tick = hoverTick;

		inputNotes.tap.lane = lane;
		inputNotes.tap.width = width;
		inputNotes.tap.tick = tick;
		inputNotes.tap.flick = currentMode == TimelineMode::InsertFlick
			? edit.flickType
			: FlickType::None;
		inputNotes.tap.critical = currentMode == TimelineMode::MakeCritical;
		inputNotes.tap.friction = currentMode == TimelineMode::MakeFriction;

		inputNotes.holdStep.lane = lane;
		inputNotes.holdStep.width = width;
		inputNotes.holdStep.tick = tick;

		if (insertingHold) {
			inputNotes.holdEnd.lane = lane;
			inputNotes.holdEnd.width = width;
			inputNotes.holdEnd.tick = tick;
		}
		else {
			inputNotes.holdStart.lane = lane;
			inputNotes.holdStart.width = width;
			inputNotes.holdStart.tick = tick;
		}

		inputNotes.damage.lane = lane;
		inputNotes.damage.width = width;
		inputNotes.damage.tick = tick;
	}

	void ScoreEditorTimeline::insertEvent(ScoreContext& context, EditArgs& edit) {
		if (currentMode == TimelineMode::InsertBPM) {
			for (const auto& tempo : context.score.tempoChanges)
				if (tempo.tick == hoverTick)
					return;

			Score prev = context.score;
			context.score.tempoChanges.push_back({ hoverTick, edit.bpm });
			Utilities::sort<Tempo>(
				context.score.tempoChanges,
				[](const Tempo& a, const Tempo& b) { return a.tick < b.tick; });

			context.pushHistory("Insert BPM change", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertTimeSign) {
			int measure = accumulateMeasures(hoverTick, TICKS_PER_BEAT,
				context.score.timeSignatures);
			if (context.score.timeSignatures.find(measure) !=
				context.score.timeSignatures.end())
				return;

			Score prev = context.score;
			context.score.timeSignatures[measure] = {
					measure, edit.timeSignatureNumerator, edit.timeSignatureDenominator };
			context.pushHistory("Insert time signature", prev, context.score);
		}
		else if (currentMode == TimelineMode::InsertHiSpeed) {
			for (const auto& hs : context.score.hiSpeedChanges)
				if (hs.tick == hoverTick && hs.layer == context.selectedLayer)
					return;

			Score prev = context.score;
			context.score.hiSpeedChanges.push_back({ hoverTick, edit.hiSpeed, context.selectedLayer });
			context.pushHistory("Insert hi-speed changes", prev, context.score);
		}
	}

	void ScoreEditorTimeline::previewInput(const ScoreContext& context, EditArgs& edit,
		Renderer* renderer) {
		updateInputNotes(context.score, edit);
		switch (currentMode) {
		case TimelineMode::InsertLong:
			if (insertingHold) {
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear,
					false, renderer, noteTint);
				drawNote(inputNotes.holdStart, renderer, noteTint);
				drawNote(inputNotes.holdEnd, renderer, noteTint);
			}
			else {
				drawNote(inputNotes.holdStart, renderer, hoverTint);
			}
			break;

		case TimelineMode::InsertLongMid:
			drawHoldMid(inputNotes.holdStep, edit.stepType, renderer, hoverTint);
			drawOutline(
				StepDrawData(inputNotes.holdStep, edit.stepType == HoldStepType::Skip
					? StepDrawType::SkipStep
					: StepDrawType::NormalStep));
			break;

		case TimelineMode::InsertGuide:
			if (insertingHold) {
				drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear,
					true, renderer, noteTint, 1, 0);
				drawOutline(
					StepDrawData(inputNotes.holdStart, StepDrawType::GuideGreen));
				drawOutline(
					StepDrawData(inputNotes.holdEnd, StepDrawType::GuideGreen));
			}
			else {
				drawOutline(
					StepDrawData(inputNotes.holdStart, StepDrawType::GuideGreen));
			}
			break;

		case TimelineMode::InsertBPM:
			bpmControl(context.score, edit.bpm, hoverTick, false);
			break;

		case TimelineMode::InsertTimeSign:
			timeSignatureControl(context.score, edit.timeSignatureNumerator,
				edit.timeSignatureDenominator, hoverTick, false);
			break;

		case TimelineMode::InsertHiSpeed:
			hiSpeedControl(context, hoverTick, edit.hiSpeed, -1);
			break;

		case TimelineMode::InsertDamage:
			drawCcNote(inputNotes.damage, renderer, hoverTint);
			break;

		default:
			drawNote(inputNotes.tap, renderer, hoverTint);
			break;
		}
	}

	void ScoreEditorTimeline::executeInput(ScoreContext& context, EditArgs& edit) {
		context.showAllLayers = false;
		switch (currentMode) {
		case TimelineMode::InsertTap:
		case TimelineMode::MakeCritical:
		case TimelineMode::MakeFriction:
		case TimelineMode::InsertFlick:
			insertNote(context, edit);
			break;

		case TimelineMode::InsertLong:
		case TimelineMode::InsertGuide:
			insertingHold = true;
			break;

		case TimelineMode::InsertLongMid: {
			int id = findClosestHold(context, hoverLane, hoverTick);
			if (id != -1)
				insertHoldStep(context, edit, id);
		} break;

		case TimelineMode::InsertBPM:
		case TimelineMode::InsertTimeSign:
		case TimelineMode::InsertHiSpeed:
			insertEvent(context, edit);
			break;

		case TimelineMode::InsertDamage:
			insertDamage(context, edit);
			break;

		default:
			break;
		}
	}

	void ScoreEditorTimeline::changeMode(TimelineMode mode, EditArgs& edit) {
		if (currentMode == mode) {
			if (mode == TimelineMode::InsertLongMid) {
				edit.stepType = (HoldStepType)(((int)edit.stepType + 1) %
					(int)HoldStepType::HoldStepTypeCount);
			}
			else if (mode == TimelineMode::InsertFlick) {
				edit.flickType = (FlickType)(((int)edit.flickType + 1) %
					(int)FlickType::FlickTypeCount);
				if (!(int)edit.flickType)
					edit.flickType = FlickType::Default;
			}
		}

		currentMode = mode;
	}

	int ScoreEditorTimeline::findClosestHold(ScoreContext& context, int lane,
		int tick) {
		float xt = laneToPosition(lane);
		float yt = getNoteYPosFromTick(tick);

		for (const auto& [id, hold] : context.score.holdNotes)
		{
			const Note& start = context.score.notes.at(hold.start.ID);
			const Note& end = context.score.notes.at(hold.end);

			// No need to search holds outside the cursor's reach
			if (start.tick > tick || end.tick < tick)
				continue;

			// Do not take skip steps into account
			int s1{ -1 }, s2{ -1 };
			while (++s2 < hold.steps.size())
			{
				if (hold.steps[s2].type != HoldStepType::Skip)
					break;
			}

			if (isArrayIndexInBounds(s2, hold.steps))
			{
				// Getting here means we found a non-skip step
				if (isMouseInHoldPath(start, context.score.notes.at(hold.steps[s2].ID), hold.start.ease, xt, yt))
					return id;

				s1 = s2;
				while (++s2 < hold.steps.size())
				{
					if (hold.steps[s2].type != HoldStepType::Skip)
					{
						const Note& m1 = context.score.notes.at(hold.steps[s1].ID);
						const Note& m2 = context.score.notes.at(hold.steps[s2].ID);
						if (isMouseInHoldPath(m1, m2, hold.steps[s1].ease, xt, yt))
							return id;

						s1 = s2;
					}
				}

				if (isMouseInHoldPath(context.score.notes.at(hold.steps[s1].ID), end, hold.steps[s1].ease, xt, yt))
					return id;
			}
			else
			{
				// Hold consists of only skip steps or no steps at all
				if (isMouseInHoldPath(start, end, hold.start.ease, xt, yt))
					return id;
			}
		}

		return -1;
	}

	bool ScoreEditorTimeline::noteControl(ScoreContext& context, const ImVec2& pos,
		const ImVec2& sz, const char* id,
		ImGuiMouseCursor cursor) {
		int minLane = MIN_LANE - context.score.metadata.laneExtension;
		int maxLane = MAX_LANE + context.score.metadata.laneExtension;
		// Do not process notes if the cursor is outside of the timeline
		// This fixes ui buttons conflicting with note "buttons"
		if (!mouseInTimeline && !isHoldingNote)
			return false;

		// Do not allow editing notes during playback
		if (playing)
			return false;

		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, sz);
		if (mouseInTimeline && ImGui::IsItemHovered() && !dragging)
			ImGui::SetMouseCursor(cursor);

		// Note clicked
		if (ImGui::IsItemActivated())
		{
			prevUpdateScore = context.score;
			ctrlMousePos = mousePos;
			holdLane = hoverLane;
			holdTick = hoverTick;
		}

		// Holding note
		if (ImGui::IsItemActive())
		{
			ImGui::SetMouseCursor(cursor);
			isHoldingNote = true;
			return true;
		}

		// Note released
		if (ImGui::IsItemDeactivated())
		{
			isHoldingNote = false;
			if (hasEdit) {
				std::unordered_set<int> sortHolds = context.getHoldsFromSelection();
				for (int id : sortHolds) {
					HoldNote& hold = context.score.holdNotes.at(id);
					Note& start = context.score.notes.at(id);
					Note& end = context.score.notes.at(hold.end);

					if (start.tick > end.tick) {
						std::swap(start.tick, end.tick);
						std::swap(start.lane, end.lane);
						std::swap(start.width, end.width);
					}

					if (hold.steps.size()) {
						sortHoldSteps(context.score, hold);

						// Ensure hold steps are between the start and end
						Note& firstMid = context.score.notes.at(hold.steps[0].ID);
						if (start.tick > firstMid.tick) {
							std::swap(start.tick, firstMid.tick);
							std::swap(start.lane, firstMid.lane);
							start.lane =
								std::clamp(start.lane, minLane, maxLane - start.width + 1);
							firstMid.lane = std::clamp(firstMid.lane, minLane,
								maxLane - firstMid.width + 1);
						}

						Note& lastMid =
							context.score.notes.at(hold.steps[hold.steps.size() - 1].ID);
						if (end.tick < lastMid.tick) {
							std::swap(end.tick, lastMid.tick);
							std::swap(end.lane, lastMid.lane);
							lastMid.lane =
								std::clamp(lastMid.lane, minLane, maxLane - lastMid.width + 1);
							end.lane = std::clamp(end.lane, minLane, maxLane - end.width + 1);
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

	void ScoreEditorTimeline::updateNote(ScoreContext& context, EditArgs& edit,
		Note& note) {
		const int minLane = MIN_LANE - context.score.metadata.laneExtension;
		const int maxLane = MAX_LANE + context.score.metadata.laneExtension;
		const int maxNoteWidth =
			MAX_NOTE_WIDTH + context.score.metadata.laneExtension * 2;

		const float btnPosY = position.y - tickToPosition(note.tick) + visualOffset -
			(notesHeight * 0.5f);
		float btnPosX = laneToPosition(note.lane) + position.x - 2.0f;

		ImVec2 pos{ btnPosX, btnPosY };
		ImVec2 noteSz{ laneToPosition(note.lane + note.width) + position.x + 2.0f -
											btnPosX,
									notesHeight };
		ImVec2 sz{ noteControlWidth, notesHeight };

		const ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsMouseHoveringRect(pos, pos + noteSz, false) && mouseInTimeline && (context.showAllLayers || note.layer == context.selectedLayer)) {
			isHoveringNote = true;

			float noteYDistance = std::abs(
				(btnPosY + notesHeight / 2 - visualOffset - position.y) - mousePos.y);
			if (noteYDistance < minNoteYDistance || io.KeyCtrl) {
				minNoteYDistance = noteYDistance;
				hoveringNote = note.ID;
				if (ImGui::IsMouseClicked(0) && !UI::isAnyPopupOpen()) {
					if (!io.KeyCtrl && !io.KeyAlt && !context.isNoteSelected(note))
						context.selectedNotes.clear();

					context.selectedNotes.insert(note.ID);

					if (io.KeyAlt && context.isNoteSelected(note))
						context.selectedNotes.erase(note.ID);
				}
			}
		}

		// Left resize
		ImGui::PushID(note.ID);
		if (noteControl(context, pos, sz, "L", ImGuiMouseCursor_ResizeEW)) {
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), minLane, maxLane);
			int diff = curLane - grabLane;

			if (abs(diff) > 0) {
				bool canResize = !std::any_of(
					context.selectedNotes.begin(), context.selectedNotes.end(),
					[&context, diff, minLane, maxLane, maxNoteWidth](int id) {
						Note& n = context.score.notes.at(id);
						int newLane = n.lane + diff;
						int newWidth = n.width - diff;
						return (newLane < minLane || newLane + newWidth - 1 > maxLane ||
							!isWithinRange(newWidth, MIN_NOTE_WIDTH, maxNoteWidth));
					});

				if (canResize) {
					ctrlMousePos.x = mousePos.x;
					hasEdit = true;
					for (int id : context.selectedNotes) {
						Note& n = context.score.notes.at(id);
						n.width = std::clamp(n.width - diff, MIN_NOTE_WIDTH, maxNoteWidth);
						n.lane = std::clamp(n.lane + diff, minLane, maxLane - n.width + 1);
					}
				}
			}
		}

		pos.x += noteControlWidth;
		sz.x = (laneWidth * note.width) + 4.0f - (noteControlWidth * 2.0f);

		// Move
		if (noteControl(context, pos, sz, "M", ImGuiMouseCursor_ResizeAll))
		{
			int curLane = positionToLane(mousePos.x);
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int grabTick = snapTickFromPos(-ctrlMousePos.y);

			int diff = curLane - grabLane;
			if (abs(diff) > 0) {
				isMovingNote = true;
				ctrlMousePos.x = mousePos.x;
				bool canMove = !std::any_of(
					context.selectedNotes.begin(), context.selectedNotes.end(),
					[&context, diff, minLane, maxLane](int id) {
						Note& n = context.score.notes.at(id);
						int newLane = n.lane + diff;
						return (newLane < minLane || newLane + n.width - 1 > maxLane);
					});

				if (canMove) {
					hasEdit = true;
					for (int id : context.selectedNotes) {
						Note& n = context.score.notes.at(id);
						n.lane = std::clamp(n.lane + diff, minLane, maxLane - n.width + 1);
					}
				}
			}

			diff = hoverTick - grabTick;
			if (abs(diff) > 0) {
				isMovingNote = true;
				ctrlMousePos.y = mousePos.y;

				bool canMove =
					!std::any_of(context.selectedNotes.begin(),
						context.selectedNotes.end(), [&context, diff](int id) {
							return context.score.notes.at(id).tick + diff < 0;
						});

				if (canMove) {
					hasEdit = true;
					for (int id : context.selectedNotes) {
						Note& n = context.score.notes.at(id);
						n.tick = std::max(n.tick + diff, 0);
					}
				}
			}
		}

		// Per note options here
		if (ImGui::IsItemDeactivated())
		{
			if (!isMovingNote && !context.selectedNotes.empty())
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

				case TimelineMode::InsertGuide:
					context.setGuideColor(GuideColor::GuideColorCount);
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

		// Right resize
		if (noteControl(context, pos, sz, "R", ImGuiMouseCursor_ResizeEW))
		{
			int grabLane = std::clamp(positionToLane(ctrlMousePos.x), MIN_LANE, MAX_LANE);
			int curLane = positionToLane(mousePos.x);

			int diff = curLane - grabLane;
			if (abs(diff) > 0) {
				bool canResize = !std::any_of(context.selectedNotes.begin(),
					context.selectedNotes.end(),
					[&context, diff, maxLane](int id) {
						Note& n = context.score.notes.at(id);
						int newWidth = n.width + diff;
						return (newWidth < MIN_NOTE_WIDTH ||
							n.lane + newWidth - 1 > maxLane);
					});

				if (canResize) {
					ctrlMousePos.x = mousePos.x;
					hasEdit = true;
					for (int id : context.selectedNotes) {
						Note& n = context.score.notes.at(id);
						n.width =
							std::clamp(n.width + diff, MIN_NOTE_WIDTH, maxNoteWidth - n.lane);
					}
				}
			}
		}

		ImGui::PopID();
	}

	void ScoreEditorTimeline::drawHoldCurve(
		const Note& n1, const Note& n2, EaseType ease, bool isGuide,
		Renderer* renderer, const Color& tint_, const int offsetTick,
		const int offsetLane, const float startAlpha, const float endAlpha, const GuideColor guideColor,
		const int selectedLayer
	) {
		int texIndex = isGuide ? noteTextures.guideColors : noteTextures.holdPath;
		if (texIndex == -1)
			return;

		auto tint = tint_;
		const Texture& pathTex = ResourceManager::textures[texIndex];
		const int sprIndex = isGuide ? static_cast<int>(guideColor) : n1.critical ? 3 : 1;
		if (!isArrayIndexInBounds(sprIndex, pathTex.sprites))
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

		auto easeFunc = getEaseFunction(ease);
		float steps = std::max(5.0f, std::ceilf(abs((endY - startY)) / 10));

		Color inactiveTint = tint * otherLayerTint;
		for (int y = 0; y < steps; ++y) {
			const float percent1 = y / steps;
			const float percent2 = (y + 1) / steps;

			float xl1 = easeFunc(startX1, endX1, percent1) - 2;
			float xr1 = easeFunc(startX2, endX2, percent1) + 2;
			float y1 = lerp(startY, endY, percent1);
			float y2 = lerp(startY, endY, percent2);
			float xl2 = easeFunc(startX1, endX1, percent2) - 2;
			float xr2 = easeFunc(startX2, endX2, percent2) + 2;

			if (y2 <= 0)
				continue;

			// rest of hold no longer visible
			if (y1 > size.y + size.y + position.y + 100)
				break;

			Color localTint = selectedLayer == -1 ? noteTint : Color::lerp(
				n1.layer == selectedLayer ? noteTint : inactiveTint,
				n2.layer == selectedLayer ? noteTint : inactiveTint,
				percent1
			);

			localTint.a = tint.a * lerp(0.7, 1, lerp(startAlpha, endAlpha, percent1));

			Vector2 p1{ xl1, y1 };
			Vector2 p2{ xl1 + holdSliceSize, y1 };
			Vector2 p3{ xl2, y2 };
			Vector2 p4{ xl2 + holdSliceSize, y2 };
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left, left + holdSliceWidth,
				spr.getY(), spr.getY() + spr.getHeight(), localTint);

			p1.x = xl1 + holdSliceSize;
			p2.x = xr1 - holdSliceSize;
			p3.x = xl2 + holdSliceSize;
			p4.x = xr2 - holdSliceSize;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, left + holdSliceWidth,
				right - holdSliceWidth, spr.getY(),
				spr.getY() + spr.getHeight(), localTint);

			p1.x = xr1 - holdSliceSize;
			p2.x = xr1;
			p3.x = xr2 - holdSliceSize;
			p4.x = xr2;
			renderer->drawQuad(p1, p2, p3, p4, pathTex, right - holdSliceWidth, right,
				spr.getY(), spr.getY() + spr.getHeight(), localTint);
		}
	}

	void ScoreEditorTimeline::drawInputNote(Renderer* renderer) {
		if (insertingHold) {
			drawHoldCurve(inputNotes.holdStart, inputNotes.holdEnd, EaseType::Linear,
				false, renderer, noteTint);
			drawNote(inputNotes.holdStart, renderer, noteTint);
			drawNote(inputNotes.holdEnd, renderer, noteTint);
		}
		else {
			drawNote(currentMode == TimelineMode::InsertLong ? inputNotes.holdStart
				: inputNotes.tap,
				renderer, hoverTint);
		}
	}

	void ScoreEditorTimeline::drawHoldNote(
		const std::unordered_map<int, Note>& notes, const HoldNote& note,
		Renderer* renderer, const Color& tint_, const int selectedLayer,
		const int offsetTicks, const int offsetLane) {
		const Note& start = notes.at(note.start.ID);
		const Note& end = notes.at(note.end);
		const int length = abs(end.tick - start.tick);
		auto tint = tint_;
		if (note.steps.size()) {
			static constexpr auto isSkipStep = [](const HoldStep& step) {
				return step.type == HoldStepType::Skip;
				};
			int s1 = -1;
			int s2 = -1;


			if (length > 0) {
				for (int i = 0; i < note.steps.size(); ++i) {
					if (isSkipStep(note.steps[i]))
						continue;

					s2 = i;
					const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
					const Note& n2 = s2 == -1 ? end : notes.at(note.steps[s2].ID);
					const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
					const float p1 = (n1.tick - start.tick) / (float)length;
					const float p2 = (n2.tick - start.tick) / (float)length;
					float a1, a2;
					if (!note.isGuide() || note.fadeType == FadeType::None) {
						a1 = 1;
						a2 = 1;
					}
					else if (note.fadeType == FadeType::In) {
						a1 = p1;
						a2 = p2;
					}
					else {
						a1 = 1 - p1;
						a2 = 1 - p2;
					}
					drawHoldCurve(n1, n2, ease, note.isGuide(), renderer, tint, offsetTicks,
						offsetLane, a1, a2, note.guideColor, selectedLayer);

					s1 = s2;
				}

				const float p1 = s1 == -1 ? 0 : (notes.at(note.steps[s1].ID).tick - start.tick) / (float)length;
				const float p2 = 1;
				float a1, a2;
				if (!note.isGuide() || note.fadeType == FadeType::None) {
					a1 = 1;
					a2 = 1;
				}
				else if (note.fadeType == FadeType::In) {
					a1 = p1;
					a2 = p2;
				}
				else {
					a1 = 1 - p1;
					a2 = 1 - p2;
				}

				const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
				const EaseType ease = s1 == -1 ? note.start.ease : note.steps[s1].ease;
				drawHoldCurve(n1, end, ease, note.isGuide(), renderer, tint, offsetTicks,
					offsetLane, a1, a2, note.guideColor, selectedLayer);
			}

			s1 = -1;
			s2 = 1;

			if (noteTextures.notes == -1)
				return;

			const Texture& tex = ResourceManager::textures[noteTextures.notes];
			const Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };
			for (int i = 0; i < note.steps.size(); ++i) {
				const Note& n3 = notes.at(note.steps[i].ID);

				// find first non-skip step
				s2 = std::distance(note.steps.cbegin(),
					std::find_if(note.steps.cbegin() + std::max(s2, i),
						note.steps.cend(),
						std::not_fn(isSkipStep)));

				if (isNoteVisible(n3, offsetTicks)) {
					if (drawHoldStepOutlines)
						drawSteps.emplace_back(StepDrawData{
								n3.tick + offsetTicks, n3.lane + offsetLane, n3.width,
								note.steps[i].type == HoldStepType::Skip
										? StepDrawType::SkipStep
										: StepDrawType::NormalStep,
								n3.layer
							});

					if (note.steps[i].type != HoldStepType::Hidden) {
						int sprIndex = getNoteSpriteIndex(n3);
						if (sprIndex > -1 && sprIndex < tex.sprites.size()) {
							const Sprite& s = tex.sprites[sprIndex];
							Vector2 pos{
									laneToPosition(n3.lane + offsetLane + (n3.width / 2.0f)),
									getNoteYPosFromTick(n3.tick + offsetTicks) };

							if (isSkipStep(note.steps[i])) {
								// find the note before and after the skip step
								const Note& n1 = s1 == -1 ? start : notes.at(note.steps[s1].ID);
								const Note& n2 =
									s2 >= note.steps.size() ? end : notes.at(note.steps[s2].ID);

								// calculate the interpolation ratio based on the distance between
								// n1 and n2
								float ratio =
									(float)(n3.tick - n1.tick) / (float)(n2.tick - n1.tick);
								const EaseType rEase =
									s1 == -1 ? note.start.ease : note.steps[s1].ease;

								auto easeFunc = getEaseFunction(rEase);

								// interpolate the step's position
								float x1 = easeFunc(laneToPosition(n1.lane + offsetLane),
									laneToPosition(n2.lane + offsetLane), ratio);
								float x2 = easeFunc(
									laneToPosition(n1.lane + offsetLane + n1.width),
									laneToPosition(n2.lane + offsetLane + n2.width), ratio);
								pos.x = midpoint(x1, x2);
							}

							renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter,
								tex, s.getX(), s.getX() + s.getWidth(),
								s.getY(), s.getY() + s.getHeight(),
								(selectedLayer == -1 || n3.layer == selectedLayer) ? tint : tint * otherLayerTint,
								1);
						}
					}
				}

				// update first interpolation point
				if (!isSkipStep(note.steps[i]))
					s1 = i;
			}
		}
		else {
			float a1, a2;
			if (!note.isGuide() || note.fadeType == FadeType::None) {
				a1 = 1;
				a2 = 1;
			}
			else if (note.fadeType == FadeType::In) {
				a1 = 0;
				a2 = 1;
			}
			else {
				a1 = 1;
				a2 = 0;
			}
			drawHoldCurve(start, end, note.start.ease, note.isGuide(), renderer, tint,
				offsetTicks, offsetLane, a1, a2, note.guideColor, selectedLayer);
		}

		auto inactiveTint = tint * otherLayerTint;

		if (isNoteVisible(start, offsetTicks)) {
			if (note.startType == HoldNoteType::Normal) {
				drawNote(start, renderer, (selectedLayer == -1 || start.layer == selectedLayer) ? tint : inactiveTint, offsetTicks, offsetLane);
			}
			else if (drawHoldStepOutlines) {
				StepDrawType drawType;
				if (note.startType == HoldNoteType::Guide) {
					drawType = static_cast<StepDrawType>(static_cast<int>(StepDrawType::GuideNeutral) + static_cast<int>(note.guideColor));
				}
				else {
					drawType = start.critical ? StepDrawType::InvisibleHoldCritical
						: StepDrawType::InvisibleHold;
				}
				drawSteps.push_back({ start.tick + offsetTicks, start.lane + offsetLane,
														 start.width,
														 drawType,
														 start.layer });
			}
		}

		if (isNoteVisible(end, offsetTicks)) {
			if (note.endType == HoldNoteType::Normal) {
				drawNote(end, renderer, (selectedLayer == -1 || end.layer == selectedLayer) ? tint : inactiveTint, offsetTicks, offsetLane);
			}
			else if (drawHoldStepOutlines) {
				StepDrawType drawType;
				if (note.startType == HoldNoteType::Guide) {
					drawType = static_cast<StepDrawType>(static_cast<int>(StepDrawType::GuideNeutral) + static_cast<int>(note.guideColor));
				}
				else {
					drawType = start.critical ? StepDrawType::InvisibleHoldCritical
						: StepDrawType::InvisibleHold;
				}
				drawSteps.push_back({ end.tick + offsetTicks, end.lane + offsetLane,
														 end.width,
														 drawType,
														 end.layer });
			}
		}
	}

	void ScoreEditorTimeline::drawHoldMid(Note& note, HoldStepType type,
		Renderer* renderer, const Color& tint) {
		if (type == HoldStepType::Hidden || noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		int sprIndex = getNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		// Center diamond
		Vector2 pos{ laneToPosition(note.lane + (note.width / 2.0f)),
								getNoteYPosFromTick(note.tick) };
		Vector2 nodeSz{ notesHeight - 5, notesHeight - 5 };

		renderer->drawSprite(pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex,
			s.getX(), s.getX() + s.getWidth(), s.getY(),
			s.getY() + s.getHeight(), tint, 1);
	}

	void ScoreEditorTimeline::drawOutline(const StepDrawData& data, int selectedLayer) {
		float x = position.x;
		float y = position.y - tickToPosition(data.tick) + visualOffset;

		ImVec2 p1{ x + laneToPosition(data.lane), y - (notesHeight * 0.15f) };
		ImVec2 p2{ x + laneToPosition(data.lane + data.width),
							y + (notesHeight * 0.15f) };
		auto fill = data.getFillColor();
		auto outline = data.getOutlineColor();

		if (selectedLayer != -1 && data.layer != -1 && selectedLayer != data.layer) {
			int fillAlpha = (fill & 0xFF000000) >> 24;
			fill = (fill & 0x00FFFFFF) | (fillAlpha / 2) << 24;
			int outlineAlpha = (outline & 0xFF000000) >> 24;
			outline = (outline & 0x00FFFFFF) | (outlineAlpha / 2) << 24;
		}

		ImGui::GetWindowDrawList()->AddRectFilled(p1, p2, fill, 1.0f,
			ImDrawFlags_RoundCornersAll);
		ImGui::GetWindowDrawList()->AddRect(p1, p2, outline, 1,
			ImDrawFlags_RoundCornersAll, 2);
	}

	void ScoreEditorTimeline::drawFlickArrow(const Note& note, Renderer* renderer,
		const Color& tint,
		const int offsetTick,
		const int offsetLane) {
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getFlickArrowSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;

		const Sprite& arrowS = tex.sprites[sprIndex];

		Vector2 pos{ 0, getNoteYPosFromTick(note.tick + offsetTick) };
		const float x1 = laneToPosition(note.lane + offsetLane);
		const float x2 = pos.x + laneToPosition(note.lane + note.width + offsetLane);
		pos.x = midpoint(x1, x2);
		pos.y += notesHeight * 0.7f; // move the arrow up a bit

		// Notes wider than 6 lanes also use flick arrow size 6
		int sizeIndex = std::min(note.width - 1, 5);
		Vector2 size{ laneWidth * flickArrowWidths[sizeIndex],
								 notesHeight * flickArrowHeights[sizeIndex] };

		float sx1 = arrowS.getX();
		float sx2 = arrowS.getX() + arrowS.getWidth();
		if (note.flick == FlickType::Right) {
			// Flip arrow to point to the right
			sx1 = arrowS.getX() + arrowS.getWidth();
			sx2 = arrowS.getX();
		}

		renderer->drawSprite(pos, 0.0f, size, AnchorType::MiddleCenter, tex, sx1, sx2,
			arrowS.getY(), arrowS.getY() + arrowS.getHeight(), tint,
			2);
	}

	void ScoreEditorTimeline::drawNote(const Note& note, Renderer* renderer,
		const Color& tint, const int offsetTick,
		const int offsetLane) {
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.notes];
		const int sprIndex = getNoteSpriteIndex(note);
		if (!isArrayIndexInBounds(sprIndex, tex.sprites))
			return;

		const Sprite& s = tex.sprites[sprIndex];

		Vector2 pos{ laneToPosition(note.lane + offsetLane),
								getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen = std::max(0.0f, (laneWidth * note.width) -
			(sliceSz.x * 2) + noteOffsetX + 5);
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		// left slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, left,
			left + noteSliceWidth, s.getY(),
			s.getY() + s.getHeight(), tint, 1);
		pos.x += sliceSz.x;

		// middle
		renderer->drawSprite(pos, 0.0f, midSz, anchor, tex, left + noteSliceWidth,
			right - noteSliceWidth, s.getY(),
			s.getY() + s.getHeight(), tint, 1);
		pos.x += midLen;

		// right slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, right - noteSliceWidth,
			right, s.getY(), s.getY() + s.getHeight(), tint, 1);

		if (note.friction) {
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (isArrayIndexInBounds(frictionSprIndex, tex.sprites))
			{
				// friction diamond is slightly smaller
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight * 0.8f, notesHeight * 0.8f };

				// diamond is always centered
				pos.x = midpoint(laneToPosition(note.lane + offsetLane),
					laneToPosition(note.lane + offsetLane + note.width));
				renderer->drawSprite(
					pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(),
					frictionSpr.getX() + frictionSpr.getWidth(), frictionSpr.getY(),
					frictionSpr.getY() + frictionSpr.getHeight(), tint, 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
	}

	void ScoreEditorTimeline::drawCcNote(const Note& note, Renderer* renderer,
		const Color& tint, const int offsetTick,
		const int offsetLane) {
		if (noteTextures.notes == -1)
			return;

		const Texture& tex = ResourceManager::textures[noteTextures.ccNotes];
		const int sprIndex = getCcNoteSpriteIndex(note);
		if (sprIndex < 0 || sprIndex >= tex.sprites.size())
			return;

		const Sprite& s = tex.sprites[sprIndex];

		Vector2 pos{ laneToPosition(note.lane + offsetLane),
								getNoteYPosFromTick(note.tick + offsetTick) };
		const Vector2 sliceSz(notesSliceSize, notesHeight);
		const AnchorType anchor = AnchorType::MiddleLeft;

		const float midLen = std::max(0.0f, (laneWidth * note.width) -
			(sliceSz.x * 2) + noteOffsetX + 5);
		const Vector2 midSz{ midLen, notesHeight };

		pos.x -= noteOffsetX;
		const int left = s.getX() + noteCutoffX;
		const int right = s.getX() + s.getWidth() - noteCutoffX;

		// left slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, left,
			left + noteSliceWidth, s.getY(),
			s.getY() + s.getHeight(), tint, 1);
		pos.x += sliceSz.x;

		// middle
		renderer->drawSprite(pos, 0.0f, midSz, anchor, tex, left + noteSliceWidth,
			right - noteSliceWidth, s.getY(),
			s.getY() + s.getHeight(), tint, 1);
		pos.x += midLen;

		// right slice
		renderer->drawSprite(pos, 0.0f, sliceSz, anchor, tex, right - noteSliceWidth,
			right, s.getY(), s.getY() + s.getHeight(), tint, 1);

		if (note.friction) {
			int frictionSprIndex = getFrictionSpriteIndex(note);
			if (frictionSprIndex >= 0 && frictionSprIndex < tex.sprites.size()) {
				// friction diamond is slightly smaller
				const Sprite& frictionSpr = tex.sprites[frictionSprIndex];
				const Vector2 nodeSz{ notesHeight * 0.8f, notesHeight * 0.8f };

				// diamond is always centered
				pos.x = midpoint(laneToPosition(note.lane + offsetLane),
					laneToPosition(note.lane + offsetLane + note.width));
				renderer->drawSprite(
					pos, 0.0f, nodeSz, AnchorType::MiddleCenter, tex, frictionSpr.getX(),
					frictionSpr.getX() + frictionSpr.getWidth(), frictionSpr.getY(),
					frictionSpr.getY() + frictionSpr.getHeight(), tint, 1);
			}
		}

		if (note.isFlick())
			drawFlickArrow(note, renderer, tint, offsetTick, offsetLane);
	}

	bool ScoreEditorTimeline::bpmControl(const Score& score, const Tempo& tempo) {
		return bpmControl(score, tempo.bpm, tempo.tick, !playing);
	}

	bool ScoreEditorTimeline::bpmControl(const Score& score, float bpm, int tick,
		bool enabled) {
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX(score) + (15 * dpiScale),
								position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineEndX(score), pos, tempoColor,
			IO::formatString("%g BPM", bpm).c_str(), enabled);
	}

	bool ScoreEditorTimeline::timeSignatureControl(const Score& score,
		int numerator, int denominator,
		int tick, bool enabled) {
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX(score) + (78 * dpiScale),
								position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineEndX(score), pos, timeColor,
			IO::formatString("%d/%d", numerator, denominator).c_str(),
			enabled);
	}

	bool ScoreEditorTimeline::skillControl(const Score& score,
		const SkillTrigger& skill) {
		return skillControl(score, skill.tick, !playing);
	}

	bool ScoreEditorTimeline::skillControl(const Score& score, int tick,
		bool enabled) {
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(score) - (50 * dpiScale),
								position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(score), pos, skillColor,
			getString("skill"), enabled);
	}

	bool ScoreEditorTimeline::feverControl(const Score& score, const Fever& fever) {
		return feverControl(score, fever.startTick, true, !playing) ||
			feverControl(score, fever.endTick, false, !playing);
	}

	bool ScoreEditorTimeline::feverControl(const Score& score, int tick, bool start,
		bool enabled) {
		if (tick < 0)
			return false;

		std::string txt = "FEVER";
		txt.append(start ? ICON_FA_CARET_UP : ICON_FA_CARET_DOWN);

		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineStartX(score) - (108 * dpiScale),
								position.y - tickToPosition(tick) + visualOffset };
		return eventControl(getTimelineStartX(score), pos, feverColor, txt.c_str(),
			enabled);
	}

	bool ScoreEditorTimeline::hiSpeedControl(const ScoreContext& context,
		const HiSpeedChange& hiSpeed) {
		return hiSpeedControl(context, hiSpeed.tick, hiSpeed.speed, hiSpeed.layer);
	}

	bool ScoreEditorTimeline::hiSpeedControl(const ScoreContext& context, int tick,
		float speed, int layer) {
		std::string txt = (layer == -1 || context.selectedLayer == layer) ? IO::formatString("%.2fx", speed) : IO::formatString("%.2fx (%s)", speed, context.score.layers[layer].name.c_str());
		float dpiScale = ImGui::GetMainViewport()->DpiScale;
		Vector2 pos{ getTimelineEndX(context.score) + (((layer == -1 || context.showAllLayers || context.selectedLayer == layer) ? 123 : 180) * dpiScale),
								position.y - tickToPosition(tick) + visualOffset };
		bool enabled = layer == -1 || context.showAllLayers || context.selectedLayer == layer;
		return eventControl(getTimelineEndX(context.score), pos,
			enabled ? speedColor : inactiveSpeedColor,
			txt.c_str(), enabled);
	}

	void ScoreEditorTimeline::eventEditor(ScoreContext& context) {
		ImGui::SetNextWindowSize(ImVec2(250, -1), ImGuiCond_Always);
		if (ImGui::BeginPopup("edit_event")) {
			std::string editLabel{ "edit_" };
			editLabel.append(eventTypes[(int)eventEdit.type]);
			ImGui::Text(getString(editLabel));
			ImGui::Separator();

			if (eventEdit.type == EventType::Bpm)
			{
				if (!isArrayIndexInBounds(eventEdit.editIndex, context.score.tempoChanges))
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();

				Tempo& tempo = context.score.tempoChanges[eventEdit.editIndex];
				UI::addFloatProperty(getString("bpm"), eventEdit.editBpm, "%g");
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					Score prev = context.score;
					tempo.bpm = std::clamp(eventEdit.editBpm, MIN_BPM, MAX_BPM);

					context.pushHistory("Change tempo", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first tempo change
				if (tempo.tick != 0) {
					ImGui::Separator();
					if (ImGui::Button(getString("remove"),
						ImVec2(-1, UI::btnSmall.y + 2))) {
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.tempoChanges.erase(context.score.tempoChanges.begin() +
							eventEdit.editIndex);
						context.pushHistory("Remove tempo change", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::TimeSignature)
			{
				if (context.score.timeSignatures.find(eventEdit.editIndex) == context.score.timeSignatures.end())
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				if (UI::timeSignatureSelect(eventEdit.editTimeSignatureNumerator,
					eventEdit.editTimeSignatureDenominator)) {
					Score prev = context.score;
					TimeSignature& ts = context.score.timeSignatures[eventEdit.editIndex];
					ts.numerator = std::clamp(abs(eventEdit.editTimeSignatureNumerator), MIN_TIME_SIGNATURE, MAX_TIME_SIGNATURE_NUMERATOR);
					ts.denominator = std::clamp(abs(eventEdit.editTimeSignatureDenominator), MIN_TIME_SIGNATURE, MAX_TIME_SIGNATURE_DENOMINATOR);

					context.pushHistory("Change time signature", prev, context.score);
				}
				UI::endPropertyColumns();

				// cannot remove the first time signature
				if (eventEdit.editIndex != 0) {
					ImGui::Separator();
					if (ImGui::Button(getString("remove"),
						ImVec2(-1, UI::btnSmall.y + 2))) {
						ImGui::CloseCurrentPopup();
						Score prev = context.score;
						context.score.timeSignatures.erase(eventEdit.editIndex);
						context.pushHistory("Remove time signature", prev, context.score);
					}
				}
			}
			else if (eventEdit.type == EventType::HiSpeed)
			{
				if (!isArrayIndexInBounds(eventEdit.editIndex, context.score.hiSpeedChanges))
				{
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}

				UI::beginPropertyColumns();
				UI::addFloatProperty(getString("hi_speed_speed"), eventEdit.editHiSpeed,
					"%g");
				HiSpeedChange& hiSpeed =
					context.score.hiSpeedChanges[eventEdit.editIndex];
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					Score prev = context.score;
					hiSpeed.speed = eventEdit.editHiSpeed;

					context.pushHistory("Change hi-speed", prev, context.score);
				}
				UI::endPropertyColumns();

				ImGui::Separator();
				if (ImGui::Button(getString("remove"), ImVec2(-1, UI::btnSmall.y + 2))) {
					ImGui::CloseCurrentPopup();
					Score prev = context.score;
					context.score.hiSpeedChanges.erase(
						context.score.hiSpeedChanges.begin() + eventEdit.editIndex);
					context.pushHistory("Remove hi-speed change", prev, context.score);
				}
			}
			ImGui::EndPopup();
		}
	}

	bool ScoreEditorTimeline::isMouseInHoldPath(const Note& n1, const Note& n2,
		EaseType ease, float x, float y) {
		float xStart1 = laneToPosition(n1.lane);
		float xStart2 = laneToPosition(n1.lane + n1.width);
		float xEnd1 = laneToPosition(n2.lane);
		float xEnd2 = laneToPosition(n2.lane + n2.width);
		float y1 = getNoteYPosFromTick(n1.tick);
		float y2 = getNoteYPosFromTick(n2.tick);

		if (!isWithinRange(y, y1, y2))
			return false;

		auto easeFunc = getEaseFunction(ease);
		float percent = (y - y1) / (y2 - y1);
		float x1 = easeFunc(xStart1, xEnd1, percent);
		float x2 = easeFunc(xStart2, xEnd2, percent);

		return isWithinRange(x, std::min(x1, x2), std::max(x1, x2));
	}

	void ScoreEditorTimeline::previousTick(ScoreContext& context) {
		if (playing || context.currentTick == 0)
			return;

		context.currentTick = std::max(roundTickDown(context.currentTick, division) -
			(TICKS_PER_BEAT / (division / 4)),
			0);
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Down);
	}

	void ScoreEditorTimeline::nextTick(ScoreContext& context) {
		if (playing)
			return;

		context.currentTick = roundTickDown(context.currentTick, division) +
			(TICKS_PER_BEAT / (division / 4));
		lastSelectedTick = context.currentTick;
		focusCursor(context, Direction::Up);
	}

	int ScoreEditorTimeline::roundTickDown(int tick, int division) {
		return std::max(tick - (tick % (TICKS_PER_BEAT / (division / 4))), 0);
	}

	void ScoreEditorTimeline::insertNote(ScoreContext& context, EditArgs& edit) {
		Score prev = context.score;

		Note newNote = inputNotes.tap;
		newNote.ID = nextID++;
		newNote.layer = context.selectedLayer;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert note", prev, context.score);
	}

	void ScoreEditorTimeline::insertHold(ScoreContext& context, EditArgs& edit) {
		Score prev = context.score;

		Note holdStart = inputNotes.holdStart;
		holdStart.ID = nextID++;
		holdStart.layer = context.selectedLayer;

		Note holdEnd = inputNotes.holdEnd;
		holdEnd.ID = nextID++;
		holdEnd.parentID = holdStart.ID;
		holdEnd.layer = context.selectedLayer;

		if (holdStart.tick == holdEnd.tick)
		{
			holdEnd.tick += TICKS_PER_BEAT / (static_cast<float>(division) / 4);
		}
		else if (holdStart.tick > holdEnd.tick) {
			std::swap(holdStart.tick, holdEnd.tick);
			std::swap(holdStart.width, holdEnd.width);
			std::swap(holdStart.lane, holdEnd.lane);
		}

		HoldNoteType holdType = currentMode == TimelineMode::InsertGuide
			? HoldNoteType::Guide
			: HoldNoteType::Normal;
		context.score.notes[holdStart.ID] = holdStart;
		context.score.notes[holdEnd.ID] = holdEnd;
		context.score.holdNotes[holdStart.ID] = {
				{holdStart.ID, HoldStepType::Normal, edit.easeType},
				{},
				holdEnd.ID,
				holdType,
				holdType };
		context.pushHistory("Insert hold", prev, context.score);
	}

	void ScoreEditorTimeline::insertHoldStep(ScoreContext& context, EditArgs& edit,
		int holdId) {
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
		holdStep.layer = context.selectedLayer;

		context.score.notes[holdStep.ID] = holdStep;

		hold.steps.push_back({ holdStep.ID,
													hold.isGuide() ? HoldStepType::Hidden : edit.stepType,
													edit.easeType });

		// sort steps in-case the step is inserted before/after existing steps
		sortHoldSteps(context.score, hold);
		context.pushHistory("Insert hold step", prev, context.score);
	}

	void ScoreEditorTimeline::insertDamage(ScoreContext& context, EditArgs& edit) {
		Score prev = context.score;

		Note newNote = inputNotes.damage;
		newNote.ID = nextID++;
		newNote.layer = context.selectedLayer;

		context.score.notes[newNote.ID] = newNote;
		context.pushHistory("Insert damage", prev, context.score);
	}

	void ScoreEditorTimeline::debug() {
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
	}

	ScoreEditorTimeline::ScoreEditorTimeline() {
		framebuffer = std::make_unique<Framebuffer>(1920, 1080);

		background.load(
			config.backgroundImage.empty()
			? (Application::getAppDir() + "res\\textures\\default.png")
			: config.backgroundImage);
		background.setBrightness(0.67);
	}

	void ScoreEditorTimeline::setPlaying(ScoreContext& context, bool state)
	{
		if (playing == state)
			return;

		playing = state;
		if (playing)
		{
			playStartTime = time;
			context.audio.seekMusic(time);
			context.audio.syncAudioEngineTimer();
			context.audio.playMusic(time);
		}
		else {
			if (config.returnToLastSelectedTickOnPause) {
				context.currentTick = lastSelectedTick;
				offset = std::max(minOffset,
					tickToPosition(context.currentTick) +
					(size.y * (1.0f - config.cursorPositionThreshold)));
			}

			context.audio.stopSoundEffects(false);
			context.audio.stopMusic();
		}
	}

	void ScoreEditorTimeline::stop(ScoreContext& context) {
		playing = false;
		time = lastSelectedTick = context.currentTick = 0;
		offset = std::max(minOffset,
			tickToPosition(context.currentTick) +
			(size.y * (1.0f - config.cursorPositionThreshold)));

		context.audio.stopSoundEffects(false);
		context.audio.stopMusic();
	}

	void ScoreEditorTimeline::updateNoteSE(ScoreContext& context)
	{
		if (!playing)
			return;

		playingNoteSounds.clear();
		for (const auto& [id, note] : context.score.notes)
		{
			float noteTime = accumulateDuration(note.tick, TICKS_PER_BEAT, context.score.tempoChanges);
			float notePlayTime = noteTime - playStartTime;
			float offsetNoteTime = noteTime - audioLookAhead;

			static auto singleNoteSEFunc = [&context, this](const Note& note, float notePlayTime)
			{
				bool playSE = true;
				if (note.getType() == NoteType::Hold)
				{
					playSE = context.score.holdNotes.at(note.ID).startType == HoldNoteType::Normal;
				}
				else if (note.getType() == NoteType::HoldEnd)
				{
					playSE = context.score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
				}

				if (playSE)
				{
					std::string_view se = getNoteSE(note, context.score);
					std::string key = std::to_string(note.tick) + "-" + se.data();
					if (!se.empty() && (playingNoteSounds.find(key) == playingNoteSounds.end()))
					{
						context.audio.playSoundEffect(se.data(), notePlayTime, -1);
						playingNoteSounds.insert(key);
					}
				}
			};

			static auto holdNoteSEFunc = [&context, this, noteTime](const Note& note, float startTime)
			{
				int endTick = context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
				float endTime = accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);
				
				float adjustedEndTime = endTime - playStartTime + audioOffsetCorrection;
				context.audio.playSoundEffect(note.critical ? SE_CRITICAL_CONNECT : SE_CONNECT, startTime, adjustedEndTime);
			};

			if (offsetNoteTime >= timeLastFrame && offsetNoteTime < time)
			{
				singleNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
				if (note.getType() == NoteType::Hold && !context.score.holdNotes.at(note.ID).isGuide())
					holdNoteSEFunc(note, notePlayTime - audioOffsetCorrection);
			}
			else if (time == playStartTime)
			{
				// Playback just started
				if (noteTime >= time && offsetNoteTime < time)
					singleNoteSEFunc(note, notePlayTime);

				// Playback started mid-hold
				if (note.getType() == NoteType::Hold && !context.score.holdNotes.at(note.ID).isGuide())
				{
					int endTick = context.score.notes.at(context.score.holdNotes.at(note.ID).end).tick;
					float endTime = accumulateDuration(endTick, TICKS_PER_BEAT, context.score.tempoChanges);
					if ((noteTime - time) <= audioLookAhead && endTime > time)
						holdNoteSEFunc(note, std::max(0.0f, notePlayTime));
				}
			}
		}
	}

	void ScoreEditorTimeline::drawWaveform(ScoreContext& context)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		constexpr ImU32 waveformColor = 0x806C6C6C;

		// Ideally this should be calculated based on the current BPM
		const double secondsPerPixel = waveformSecondsPerPixel / zoom;
		const double durationSeconds = context.waveformL.durationInSeconds;
		const double musicOffsetInSeconds = context.workingData.musicOffset / 1000;

		for (size_t index = 0; index < 2; index++)
		{
			const Audio::WaveformMipChain& waveform = index == 0 ? context.waveformL : context.waveformR;
			const bool rightChannel = index == 1;
			if (waveform.isEmpty())
				continue;

			const Audio::WaveformMip& mip = waveform.findClosestMip(secondsPerPixel);
			for (int y = visualOffset - size.y; y < visualOffset; y += 1)
			{
				int tick = positionToTick(y);

				// Small accuracy loss by converting to ticks but shouldn't be too noticeable
				const double secondsAtPixel = accumulateDuration(tick, TICKS_PER_BEAT, context.score.tempoChanges) - musicOffsetInSeconds;
				const bool outOfBounds = secondsAtPixel < 0 || secondsAtPixel > waveform.durationInSeconds;

				float amplitude = std::max(waveform.getAmplitudeAt(mip, secondsAtPixel, secondsPerPixel), 0.0f);
				float barValue = outOfBounds ? 0.0f : (amplitude * std::min(laneWidth * 6, 180.0f));

				float timelineMidPosition = midpoint(getTimelineStartX(), getTimelineEndX());
				float rectYPosition = floorf(position.y + visualOffset - y);
				ImVec2 rect1(timelineMidPosition, rectYPosition);
				ImVec2 rect2(timelineMidPosition + (std::max(0.5f, barValue) * (rightChannel ? 1 : -1)), rectYPosition + 0.5f);
				drawList->AddRectFilled(rect1, rect2, waveformColor);
			}
		}
	}
} // namespace MikuMikuWorld
