#include "Canvas.h"
#include "Color.h"
#include "Constants.h"
#include "UI.h"
#include "Rendering/Renderer.h"
#include "ResourceManager.h"
#include "InputListener.h"
#include <algorithm>

namespace MikuMikuWorld
{
	Canvas::Canvas() : mouseInCanvas{ true }
	{
		useSmoothScrolling = true;
		smoothScrollTime = 67.0f;
		remainingScroll = scrollAmount = 0.0f;
		laneTransparency = 0.8f;
	}

	float Canvas::getNoteYPosFromTick(int tick) const
	{
		return canvasPos.y + tickToPosition(tick) - timelineVisualOffset + canvasSize.y;
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
		return y >= 0 && y <= canvasSize.y + canvasPos.y + 100;
	}

	Vector2 Canvas::getNotePos(int tick, float lane) const
	{
		return Vector2{ laneToPosition(lane), getNoteYPosFromTick(tick) };
	}

	float Canvas::getTimelineStartX() const
	{
		return canvasPos.x + laneOffset;
	}

	float Canvas::getTimelineEndX() const
	{
		return getTimelineStartX() + timelineWidth;
	}

	void Canvas::centerCursor(int cursorTick, bool playing, int mode)
	{
		float cursorPos = tickToPosition(cursorTick);
		float offset = canvasSize.y * 0.5f;

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
		timelineOffset = cursorPos + canvasSize.y;
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
		int tick = positionToTick(timelineOffset - canvasSize.y);
		float x1 = canvasPos.y - tickToPosition(tick) + timelineOffset;

		zoom = std::clamp(val, MIN_ZOOM, MAX_ZOOM);

		float x2 = canvasPos.y - tickToPosition(tick) + timelineOffset;
		timelineOffset += x1 - x2;
	}

	void Canvas::setLaneOpacity(float val)
	{
		laneTransparency = std::clamp(val, 0.0f, 1.0f);
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

	void Canvas::changeBackground(const Texture& t)
	{
		background.load(t);
		background.resize(Vector2{ canvasSize.x, canvasSize.y });
	}

	void Canvas::drawBackground(Renderer* renderer)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		int bg = background.getTextureID();
		if (!bg)
			return;

		if (canvasSize.x != prevSize.x || canvasSize.y != prevSize.y)
			background.resize(Vector2{ canvasSize.x, canvasSize.y });

		if (background.isDirty())
			background.process(renderer);

		// center background
		ImVec2 bgPos = canvasPos;
		bgPos.x -= (background.getWidth() - canvasSize.x) / 2.0f;
		bgPos.y -= (background.getHeight() - canvasSize.y) / 2.0f;
		drawList->AddImage((void*)bg, bgPos, bgPos + ImVec2(background.getWidth(), background.getHeight()));
	}

	void Canvas::drawLanesBackground()
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if (!drawList)
			return;

		drawList->AddRectFilled(
			ImVec2(getTimelineStartX(), canvasPos.y),
			ImVec2(getTimelineEndX(), canvasPos.y + canvasSize.y),
			Color::abgrToInt(laneTransparency * 255, 0x1c, 0x1a, 0x1f)
		);
	}

	void Canvas::update(float dt)
	{
		prevPos		= canvasPos;
		prevSize	= canvasSize;
		canvasSize	= ImGui::GetContentRegionAvail();
		canvasPos	= ImGui::GetCursorScreenPos();
		boundaries	= ImRect(canvasPos, canvasPos + canvasSize);
		mouseInCanvas = ImGui::IsMouseHoveringRect(canvasPos, canvasPos + canvasSize);
		
		timelineWidth = NUM_LANES * laneWidth;
		laneOffset = (canvasSize.x * 0.5f) - (timelineWidth * 0.5f);
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
	}
}
