#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

namespace MikuMikuWorld
{
	class Canvas
	{
	private:
		int laneOffset = 0.0f;
		float laneWidth = 35.0f;
		float notesHeight = 50.0f;
		float zoom = 2.0f;
		float timelineMinOffset = 0;
		float timelineOffset = 0;
		float timelineVisualOffset = 0;
		float timelineWidth;
		float effectiveTickHeight;
		float cursorPos;

		float scrollAmount;
		float remainingScroll;
		float smoothScrollTime;
		bool useSmoothScrolling;

		ImVec2 canvasSize;
		ImVec2 canvasPos;
		ImRect boundaries;

		bool mouseInCanvas;

	public:
		Canvas();

		void update(float dt);

		int positionToTick(float pos, int div = 1) const;
		float tickToPosition(int tick, int div = 1) const;
		float getNoteYPosFromTick(int tick) const;
		int positionToLane(float pos) const;
		float laneToPosition(float lane) const;
		bool isNoteInCanvas(const int tick) const;

		float getTimelineStartX() const;
		float getTimelineEndX() const;

		void updateTimelineScrollAmount();
		void updateScorllingPosition(float dt);
		void centerCursor(int cursorTick, bool playing, int mode);
		void scrollToBeginning();
		void scrollPage();

		inline bool isMouseInCanvas() { return mouseInCanvas; }
		inline bool isUseSmoothScrolling() { return useSmoothScrolling; }
		inline float getLaneWidth() { return laneWidth; }
		inline float getNotesHeight() { return notesHeight; }
		inline float getZoom() { return zoom; }
		inline float getSmoothScrollingTime() { return smoothScrollTime; }
		inline float getOffset() { return timelineOffset; }
		inline float getVisualOffset() { return timelineVisualOffset; }
		inline ImVec2 getPosition() const { return canvasPos; }
		inline ImVec2 getSize() const { return canvasSize; }
		inline ImRect getBoundaries() const { return boundaries; }

		void setLaneWidth(float width);
		void setNotesHeight(float height);
		void setZoom(float val);
		void setSmoothScrollingTime(float time);
		void setUseSmoothScrolling(bool val);
	};
}

