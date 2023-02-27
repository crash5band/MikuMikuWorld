#pragma once
#include "Background.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

namespace MikuMikuWorld
{
	struct Score;

	class Canvas
	{
	private:
		float laneOffset = 0.0f;
		float laneWidth = 35.0f;
		float notesHeight = 50.0f;
		float zoom = 2.0f;
		float timelineMaxOffset = 10000;
		float timelineMinOffset = 0;
		float timelineOffset = 0;
		float timelineVisualOffset = 0;
		float timelineWidth;
		float effectiveTickHeight;
		float laneOpacity;

		float scrollStartY;
		float scrollAmount;
		float remainingScroll;
		float smoothScrollTime;
		bool useSmoothScrolling;
		bool mouseInCanvas;

		ImVec2 size;
		ImVec2 position;
		ImVec2 prevPos;
		ImVec2 prevSize;
		ImRect boundaries;

		Background background;
	public:
		Canvas();

		void update(float dt);

		int positionToTick(float pos, int div = 1) const;
		float tickToPosition(int tick, int div = 1) const;
		float getNoteYPosFromTick(int tick) const;
		int positionToLane(float pos) const;
		float laneToPosition(float lane) const;
		bool isNoteInCanvas(const int tick) const;
		Vector2 getNotePos(int tick, float lane) const;

		float getTimelineStartX() const;
		float getTimelineEndX() const;

		void setTimelineOffset(float offset);
		void updateTimelineScrollAmount();
		void updateScorllingPosition(float dt);
		void centerCursor(int cursorTick, bool playing, int mode);
		void scrollToBeginning();
		void scrollPage(float cursorPos);
		void drawBackground(Renderer* rednerer);
		void drawLanesBackground();
		void changeBackground(const Texture& t);
		void updateTimelineScrollbar();
		void calculateMaxTimelineOffsetFromScore(const Score& score);

		inline bool isMouseInCanvas() { return mouseInCanvas; }
		inline bool isUseSmoothScrolling() { return useSmoothScrolling; }
		inline float getLaneWidth() { return laneWidth; }
		inline float getNotesHeight() { return notesHeight; }
		inline float getZoom() { return zoom; }
		inline float getLaneOpacity() { return laneOpacity; }
		inline float getBackgroundBrightness() { return background.getBrightness(); }
		inline float getSmoothScrollingTime() { return smoothScrollTime; }
		inline float getOffset() { return timelineOffset; }
		inline float getVisualOffset() { return timelineVisualOffset; }
		inline float getMinOffset() { return timelineMinOffset; }
		inline float getMaxOffset() { return timelineMaxOffset; }
		inline ImVec2 getPosition() const { return position; }
		inline ImVec2 getSize() const { return size; }
		inline ImRect getBoundaries() const { return boundaries; }

		void setLaneWidth(float width);
		void setNotesHeight(float height);
		void setZoom(float val);
		void setLaneOpacity(float val);
		void setBackgroundBrightness(float val);
		void setSmoothScrollingTime(float time);
		void setUseSmoothScrolling(bool val);
		void setTimelineMaxOffset(float offset) { timelineMaxOffset = offset; }
	};
}

