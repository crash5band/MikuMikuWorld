#include "Canvas.h"
#include "Color.h"
#include "Constants.h"
#include "UI.h"
#include "Rendering/Renderer.h"
#include "ResourceManager.h"
#include "InputListener.h"
#include "Score.h"
#include <algorithm>

namespace MikuMikuWorld
{
	Canvas::Canvas() : mouseInCanvas{ true }
	{
		useSmoothScrolling = true;
		smoothScrollTime = 67.0f;
		remainingScroll = scrollAmount = 0.0f;
		laneOpacity = 0.8f;
	}

	float Canvas::getNoteYPosFromTick(int tick) const
	{
		return position.y + tickToPosition(tick) - timelineVisualOffset + size.y;
	}

	int Canvas::positionToTick(float pos, int div) const
	{
		return roundf(pos / (TICK_HEIGHT * zoom) / div);
	}

	float Canvas::tickToPosition(int tick, int div) const
	{
		return tick * div * TICK_HEIGHT * zoom;
	}

	int Canvas::positionToLane(float pos) const
	{
		return floor((pos - laneOffset) / laneWidth);
	}

	float Canvas::laneToPosition(float lane) const
	{
		return laneOffset + (lane * laneWidth);
	}

	bool Canvas::isNoteInCanvas(const int tick) const
	{
		const float y = getNoteYPosFromTick(tick);
		return y >= 0 && y <= size.y + position.y + 100;
	}

	Vector2 Canvas::getNotePos(int tick, float lane) const
	{
		return Vector2{ laneToPosition(lane), getNoteYPosFromTick(tick) };
	}

	float Canvas::getTimelineStartX() const
	{
		return position.x + laneOffset;
	}

	float Canvas::getTimelineEndX() const
	{
		return getTimelineStartX() + timelineWidth;
	}

	void Canvas::centerCursor(int cursorTick, bool playing, int mode)
	{
		float cursorPos = tickToPosition(cursorTick);
		float offset = size.y * 0.5f;

		bool exec = false;
		switch (mode)
		{
		case 0:
			exec = true;
			break;
		case 1:
			exec = cursorPos >= timelineOffset - offset;
			break;
		case 2:
			exec = cursorPos <= timelineOffset - offset;
			break;
		default:
			break;
		}

		if (exec)
		{
			setTimelineOffset(cursorPos + offset);

			// scroll position changed
			if (!playing)
				updateTimelineScrollAmount();
		}
	}

	void Canvas::scrollToBeginning()
	{
		timelineOffset = timelineMinOffset;
		updateTimelineScrollAmount();
	}

	void Canvas::scrollPage(float cursorPos)
	{
		timelineOffset = cursorPos + size.y;
		timelineVisualOffset = timelineOffset;
	}

	void Canvas::updateTimelineScrollAmount()
	{
		scrollAmount = timelineOffset - timelineVisualOffset;
		remainingScroll = abs(scrollAmount);
	}

	void Canvas::setTimelineOffset(float offset)
	{
		timelineOffset = std::max(offset, timelineMinOffset);
	}

	void Canvas::updateScorllingPosition(float dt)
	{
		if (useSmoothScrolling)
		{
			float delta = scrollAmount / (smoothScrollTime / (dt * 1000));
			if (remainingScroll > 0.0f)
			{
				if (remainingScroll - abs(delta) < 0.0f)
				{
					timelineVisualOffset += remainingScroll;
					remainingScroll = 0;
				}
				else
				{
					timelineVisualOffset += delta;
					remainingScroll -= abs(delta);
				}
			}
			else
			{
				timelineVisualOffset = timelineOffset;
			}
		}
		else
		{
			timelineVisualOffset = timelineOffset;
		}
	}

	void Canvas::setZoom(float val)
	{
		int tick = positionToTick(timelineOffset - size.y);
		float x1 = position.y - tickToPosition(tick) + timelineOffset;

		zoom = std::clamp(val, MIN_ZOOM, MAX_ZOOM);

		float x2 = position.y - tickToPosition(tick) + timelineOffset;
		timelineOffset += x1 - x2;
	}

	void Canvas::setLaneOpacity(float val)
	{
		laneOpacity = std::clamp(val, 0.0f, 1.0f);
	}

	void Canvas::setBackgroundBrightness(float val)
	{
		background.setBrightness(val);
	}

	void Canvas::setLaneWidth(float width)
	{
		laneWidth = std::clamp((int)width, MIN_LANE_WIDTH, MAX_LANE_WIDTH);
	}

	void Canvas::setNotesHeight(float height)
	{
		notesHeight = std::clamp((int)height, MIN_NOTES_HEIGHT, MAX_NOTES_HEIGHT);
	}

	void Canvas::setUseSmoothScrolling(bool val)
	{
		useSmoothScrolling = val;

		// clear remaining scroll amount
		if (useSmoothScrolling)
		{
			timelineVisualOffset = timelineOffset;
			updateTimelineScrollAmount();
		}
	}

	void Canvas::setSmoothScrollingTime(float time)
	{
		if (time <= 150.0f && time >= 10.0f)
			smoothScrollTime = time;
	}

	void Canvas::updateTimelineScrollbar()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
		float paddingY = 30.0f;
		ImVec2 windowEndTop = ImGui::GetWindowPos() + ImVec2{ ImGui::GetWindowSize().x - scrollbarWidth - 4, paddingY };
		ImVec2 windowEndBottom = windowEndTop + ImVec2{ scrollbarWidth + 2, ImGui::GetWindowSize().y - (paddingY * 1.3f) };

		// calculate handle height
		float heightRatio = size.y / ((timelineMaxOffset - timelineMinOffset) * zoom);
		float handleHeight = std::max(20.0f, ((windowEndBottom.y - windowEndTop.y) * heightRatio) + 30.0f);
		float scrollHeight = windowEndBottom.y - windowEndTop.y - handleHeight;

		// calculate handle position
		float currentOffset = timelineOffset - timelineMinOffset;
		float positionRatio = std::min(1.0f, currentOffset / ((timelineMaxOffset * zoom) - timelineMinOffset));
		float handlePosition = windowEndBottom.y - (scrollHeight * positionRatio) - handleHeight;

		ImVec2 scrollHandleMin = ImVec2{ windowEndTop.x + 2, handlePosition };
		ImVec2 scrollHandleMax = ImVec2{ windowEndTop.x + scrollbarWidth - 2, handlePosition + handleHeight };

		// handle button
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
				float newOffset = ((timelineMaxOffset * zoom) - timelineMinOffset) * positionRatio;

				timelineOffset = newOffset + timelineMinOffset;
				scrollStartY = ImGui::GetMousePos().y;
			}
		}

		ImGui::SetCursorScreenPos(windowEndTop);
		ImGui::InvisibleButton("##scroll_background", ImVec2{ scrollbarWidth, scrollHeight + handleHeight }, ImGuiButtonFlags_AllowItemOverlap);
		if (ImGui::IsItemActivated())
		{
			float yPos = std::clamp(ImGui::GetMousePos().y, windowEndTop.y, windowEndBottom.y - handleHeight);

			// convert handle position to timeline offset
			positionRatio = std::clamp(1 - ((yPos - windowEndTop.y)) / scrollHeight, 0.0f, 1.0f);
			float newOffset = ((timelineMaxOffset * zoom) - timelineMinOffset) * positionRatio;

			timelineOffset = newOffset + timelineMinOffset;
		}

		ImU32 scrollBgColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarBg));
		ImU32 scrollHandleColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(handleColor));
		drawList->AddRectFilled(windowEndTop, windowEndBottom, scrollBgColor, 0);
		drawList->AddRectFilled(scrollHandleMin, scrollHandleMax, scrollHandleColor, ImGui::GetStyle().ScrollbarRounding, ImDrawFlags_RoundCornersAll);
	}

	void Canvas::calculateMaxTimelineOffsetFromScore(const Score& score)
	{
		int maxTick = 0;
		for (const auto& [id, note] : score.notes)
			maxTick = std::max(maxTick, note.tick);

		// current offset maybe greater than calculated offset from score
		float scoreOffset = (maxTick * TICK_HEIGHT) + timelineMinOffset + 1000;
		timelineMaxOffset = std::max(timelineOffset, scoreOffset);
	}

	void Canvas::changeBackground(const Texture& t)
	{
		background.load(t);
		background.resize(Vector2{ size.x, size.y });
	}

	void Canvas::drawBackground(Renderer* renderer)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		int bg = background.getTextureID();
		if (!bg)
			return;

		if (size.x != prevSize.x || size.y != prevSize.y)
			background.resize(Vector2{ size.x, size.y });

		if (background.isDirty())
			background.process(renderer);

		// center background
		ImVec2 bgPos = position;
		bgPos.x -= (background.getWidth() - size.x) / 2.0f;
		bgPos.y -= (background.getHeight() - size.y) / 2.0f;
		drawList->AddImage((void*)bg, bgPos, bgPos + ImVec2(background.getWidth(), background.getHeight()));
	}

	void Canvas::drawLanesBackground()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		drawList->AddRectFilled(
			ImVec2(getTimelineStartX(), position.y),
			ImVec2(getTimelineEndX(), position.y + size.y),
			Color::abgrToInt(laneOpacity * 255, 0x1c, 0x1a, 0x1f)
		);
	}

	void Canvas::update(float dt)
	{
		prevPos		= position;
		prevSize	= size;
		position	= ImGui::GetCursorScreenPos();
		size	= ImGui::GetContentRegionAvail() - ImVec2{ ImGui::GetStyle().ScrollbarSize, 0 };
		boundaries	= ImRect(position, position + size);
		mouseInCanvas = ImGui::IsMouseHoveringRect(position, position + size);
		
		timelineWidth = NUM_LANES * laneWidth;
		laneOffset = (size.x * 0.5f) - (timelineWidth * 0.5f);
		effectiveTickHeight = TICK_HEIGHT * zoom;

		// change offset to min if min offset is lower than current offset
		timelineMinOffset = ImGui::GetWindowHeight() - 100.0f;
		setTimelineOffset(timelineOffset);

		ImGui::ItemSize(boundaries);

		if (isMouseInCanvas() && !UI::isAnyPopupOpen())
		{
			float mouseWheelDelta = ImGui::GetIO().MouseWheel;
			if (InputListener::isCtrlDown())
				setZoom(zoom + (mouseWheelDelta * 0.1f));
			else
			{
				float offset = mouseWheelDelta * (InputListener::isShiftDown() ? 300.0f : 100.0f);
				setTimelineOffset(timelineOffset + offset);
				if (abs(offset) > 0.0f)
				{
					updateTimelineScrollAmount();
				}
			}
		}

		// increase scroll limit once we reach the end
		if (timelineOffset > (timelineMaxOffset - 300) * zoom)
			timelineMaxOffset += 1000;
	}
}
