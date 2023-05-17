#pragma once
#include "ScoreContext.h"
#include "ImGui/imgui_internal.h"
#include "Rendering/Camera.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/Renderer.h"
#include "TimelineMode.h"
#include "Background.h"

namespace MikuMikuWorld
{
	class ScoreEditorTimeline
	{
	private:
		TimelineMode currentMode;
		float laneOffset;
		float zoom = 1.0f;
		float maxOffset = 10000;
		float minOffset;
		float offset;
		float visualOffset;
		float unitHeight = 0.15f;

		float scrollStartY;
		bool mouseInTimeline;

		float noteControlWidth = 12;
		float minNoteYDistance;
		int hoverLane;
		int hoverTick;
		int hoveringNote;
		int holdLane;
		int holdTick;
		int lastSelectedTick;
		int division = 8;

		bool isHoveringNote;
		bool isHoldingNote;
		bool isMovingNote;
		bool skipUpdateAfterSortingSteps;
		bool dragging;
		bool insertingHold;
		bool hasEdit;

		float time;
		float timeLastFrame;
		float playStartTime;
		float songPos;
		float songPosLastFrame;
		bool playing;

		struct InputNotes
		{
			Note tap;
			Note holdStart;
			Note holdEnd;
			Note holdStep;
		} inputNotes{ Note(NoteType::Tap), Note(NoteType::Hold), Note(NoteType::HoldEnd), Note(NoteType::HoldMid) };

		struct StepDrawData
		{
			int tick;
			int lane;
			int width;
			HoldStepType type;
		};
		
		std::vector<StepDrawData> drawSteps;

		ImVec2 size;
		ImVec2 position;
		ImVec2 prevPos;
		ImVec2 prevSize;
		ImRect boundaries;

		ImVec2 ctrlMousePos;
		ImVec2 dragStart;
		ImVec2 mousePos;

		Score prevUpdateScore;

		Camera camera;
		std::unique_ptr<Framebuffer> framebuffer;
		std::unordered_map<std::string, int> tickSEMap;
		const float audioOffsetCorrection = 0.02f;
		const float audioLookAhead = 0.05f;

		void updateScrollbar();
		void updateScrollingPosition();

		void drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldNote(const std::unordered_map<int, Note>& notes, const HoldNote& note, Renderer* renderer, const Color& tint, const int offsetTicks = 0, const int offsetLane = 0);
		void drawHoldMid(Note& note, HoldStepType type, Renderer* renderer, const Color& tint);
		void drawOutline(const StepDrawData& data);
		void drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		bool noteControl(ScoreContext& context, const ImVec2& pos, const ImVec2& sz, const char* id, ImGuiMouseCursor cursor);
		bool bpmControl(const Tempo& tempo);
		bool bpmControl(float bpm, int tick, bool enabled);
		bool timeSignatureControl(int numerator, int denominator, int tick, bool enabled);
		bool skillControl(const SkillTrigger& skill);
		bool skillControl(int tick, bool enabled);
		bool feverControl(const Fever& fever);
		bool feverControl(int tick, bool start, bool enabled);
		bool hiSpeedControl(const HiSpeedChange& hiSpeed);
		bool hiSpeedControl(int tick, float speed);

		void drawInputNote(Renderer* renderer);
		void previewInput(EditArgs& edit, Renderer* renderer);
		void previewPaste(ScoreContext& context, Renderer* renderer);
		void executeInput(ScoreContext& context, EditArgs& edit);
		void eventEditor(ScoreContext& context);

		void insertNote(ScoreContext& context, EditArgs& edit, bool crticial);
		void insertHold(ScoreContext& context, EditArgs& edit);
		void insertHoldStep(ScoreContext& context, EditArgs& edit, int holdId);
		void insertEvent(ScoreContext& context, EditArgs& edit);

		void updateNoteSE(ScoreContext& context);

		void contextMenu(ScoreContext& context);

	public:
		float laneWidth = 26;
		float notesHeight = 28;
		bool drawHoldStepOutlines = true;
		Background background;

		struct EventEditParams
		{
			EventType type = EventType::None;

			int editBpmIndex = -1;
			float editBpm = 120.0f;

			int editTimeSignatureIndex = -1;
			int editTimeSignatureNumerator = 4;
			int editTimeSignatureDenominator = 4;

			int editHiSpeedIndex = -1;
			float editHiSpeed = 1.0f;
		} eventEdit {};

		int snapTickFromPos(float posY);
		int positionToTick(float pos) const;
		float tickToPosition(int tick) const;
		float getNoteYPosFromTick(int tick) const;

		int laneFromCenterPosition(int lane, int width);
		int positionToLane(float pos) const;
		float laneToPosition(float lane) const;

		constexpr inline float getTimelineStartX() const { return position.x + laneOffset; }
		constexpr inline float getTimelineEndX() const { return getTimelineStartX() + (laneWidth * 12); }

		constexpr inline float getZoom() const { return zoom; }
		void setZoom(float zoom);

		constexpr inline bool isMouseInTimeline() const { return mouseInTimeline; }
		bool isNoteVisible(const Note& note, int offsetTicks = 0) const;

		int findClosestHold(ScoreContext& context, int lane, int tick);
		bool isMouseInHoldPath(const Note& n1, const Note& n2, EaseType ease, float x, float y);
		constexpr inline bool isPlaying() const { return playing; }
		void togglePlaying(ScoreContext& context);
		void stop(ScoreContext& context);
		void calculateMaxOffsetFromScore(const Score& score);

		void update(ScoreContext& context, EditArgs& edit, Renderer* renderer);
		void updateNotes(ScoreContext& context, EditArgs& edit, Renderer* renderer);
		void updateNote(ScoreContext& context, Note& note);
		void updateInputNotes(EditArgs& edit);
		void debug();

		void previousTick(ScoreContext& context);
		void nextTick(ScoreContext& context);
		int roundTickDown(int tick, int division);
		void focusCursor(ScoreContext& context, Direction direction);

		constexpr TimelineMode getMode() const { return currentMode; }
		void changeMode(TimelineMode mode, EditArgs& edit);

		ScoreEditorTimeline();
	};
}