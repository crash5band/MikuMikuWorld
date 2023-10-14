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
	enum class StepDrawType
	{
		NormalStep,
		SkipStep,
		InvisibleHold,
		InvisibleHoldCritical,
    GuideNeutral,
    GuideRed,
    GuideGreen,
    GuideBlue,
    GuideYellow,
    GuidePurple,
    GuideCyan,
		StepDrawTypeMax
	};

	const std::array<ImU32, (int)StepDrawType::StepDrawTypeMax> stepDrawOutlineColors[] =
	{
		0xFFAAFFAA, 0xFFFFFFAA, 0xFFCCCCCC, 0xFFCCCCCC,
    0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC, 0xFFCCCCCC
	};

	const std::array<ImU32, (int)StepDrawType::StepDrawTypeMax> stepDrawFillColors[] =
	{
		0x00FFFFFF, 0x00FFFFFF, 0xFF66B622, 0xFF15A0C9,
    0xFFEDEDED, 0xFF7B73D6, 0xFF9DD673, 0xFFD67B73, 0xFF73CED6, 0xFFCD73D6, 0xFFD6AC73
	};

	class StepDrawData
	{
	public:
		int tick{};
		int lane{};
		int width{};
		StepDrawType type{};

    int layer = -1;

		StepDrawData() {}
		StepDrawData(int _tick, int _lane, int _width, StepDrawType _type, int layer = -1) :
			tick{ _tick }, lane{ _lane }, width{ _width }, type{ _type }, layer{ layer }
		{

		}

		StepDrawData(const Note& n, StepDrawType _type, int layer = -1) :
			tick{ n.tick }, lane{ n.lane }, width{ n.width }, type{ _type }, layer{ layer }
		{

		}

		inline constexpr ImU32 getFillColor() const { return stepDrawFillColors->at((int)type); }
		inline constexpr ImU32 getOutlineColor() const { return stepDrawOutlineColors->at((int)type); }
	};

	class ScoreEditorTimeline
	{
	private:
		int noteCutoffX		= 30;
		int noteSliceWidth	= 90;
		int noteOffsetX		= 5;
		int notesSliceSize	= 18;
		int holdCutoffX		= 33;
		int holdSliceWidth	= 10;
		int holdSliceSize	= 5;

		TimelineMode currentMode;
		float laneOffset;
		float zoom = 1.0f;
		float maxOffset = 10000;
		float minOffset;
		float offset;
		float visualOffset;
		const float unitHeight = 0.15f;
		const float scrollUnit = 50;

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
      Note damage;
		} inputNotes{ Note(NoteType::Tap), Note(NoteType::Hold), Note(NoteType::HoldEnd), Note(NoteType::HoldMid), Note(NoteType::Damage) };

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
		std::unordered_set<std::string> playingNoteSounds;
		const float audioOffsetCorrection = 0.02f;
		const float audioLookAhead = 0.05f;

		void updateScrollbar();
		void updateScrollingPosition();

		void drawHoldCurve(
        const Note& n1, const Note& n2,
        EaseType ease, bool isGuide, Renderer* renderer, const Color& tint,
        const int offsetTick = 0, const int offsetLane = 0, const float startAlpha = 1, const float endAlpha = 1,
        const GuideColor guideColor = GuideColor::Green, const int selectedLayer = -1
      );
		void drawHoldNote(const std::unordered_map<int, Note>& notes, const HoldNote& note, Renderer* renderer, const Color& tint, const int selectedLayer = -1, const int offsetTicks = 0, const int offsetLane = 0);
		void drawHoldMid(Note& note, HoldStepType type, Renderer* renderer, const Color& tint);
		void drawOutline(const StepDrawData& data, const int selectedLayer = -1);
		void drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawCcNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		bool noteControl(ScoreContext& context, const ImVec2& pos, const ImVec2& sz, const char* id, ImGuiMouseCursor cursor);
		bool bpmControl(const Score& score, const Tempo& tempo);
		bool bpmControl(const Score& score, float bpm, int tick, bool enabled);
		bool timeSignatureControl(const Score& score, int numerator, int denominator, int tick, bool enabled);
		bool skillControl(const Score& score, const SkillTrigger& skill);
		bool skillControl(const Score& score, int tick, bool enabled);
		bool feverControl(const Score& score, const Fever& fever);
		bool feverControl(const Score& score, int tick, bool start, bool enabled);
		bool hiSpeedControl(const ScoreContext& context, const HiSpeedChange& hiSpeed);
		bool hiSpeedControl(const ScoreContext& context, int tick, float speed, int layer);

		void drawInputNote(Renderer* renderer);
		void previewInput(const ScoreContext& context, EditArgs& edit, Renderer* renderer);
		void previewPaste(ScoreContext& context, Renderer* renderer);
		void executeInput(ScoreContext& context, EditArgs& edit);
		void eventEditor(ScoreContext& context);

		void insertNote(ScoreContext& context, EditArgs& edit);
		void insertHold(ScoreContext& context, EditArgs& edit);
		void insertHoldStep(ScoreContext& context, EditArgs& edit, int holdId);
		void insertEvent(ScoreContext& context, EditArgs& edit);
		void insertDamage(ScoreContext& context, EditArgs& edit);

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
			int editIndex = -1;

			float editBpm = 120.0f;
			int editTimeSignatureNumerator = 4;
			int editTimeSignatureDenominator = 4;
			float editHiSpeed = 1.0f;
		} eventEdit {};

		int snapTickFromPos(float posY, const ScoreContext& context);
		int positionToTick(float pos) const;
		float tickToPosition(int tick) const;
		float getNoteYPosFromTick(int tick) const;

		int laneFromCenterPosition(const Score& score, int lane, int width);
		int positionToLane(float pos) const;
		float laneToPosition(float lane) const;

		const inline float getTimelineStartX() const { return position.x + laneOffset; }
		const inline float getTimelineEndX() const { return getTimelineStartX() + (laneWidth * 12); }
		const inline float getTimelineStartX(const Score& score) const { return getTimelineStartX() - (laneWidth * score.metadata.laneExtension); }
		const inline float getTimelineEndX(const Score& score) const { return getTimelineEndX() + (laneWidth * score.metadata.laneExtension); }

		constexpr inline float getZoom() const { return zoom; }
		void setZoom(float zoom);

		constexpr inline int getDivision() const { return division; }
		void setDivision(int div) { division = std::clamp(div, 4, 1920); }

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
		void updateNote(ScoreContext& context, EditArgs& edit, Note& note);
		void updateInputNotes(const Score& score, EditArgs& edit);
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
