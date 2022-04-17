#pragma once
#include "Score.h"
#include "Tempo.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Rendering/Camera.h"
#include "Rendering/Framebuffer.h"
#include "Constants.h"
#include "Note.h"
#include "TimelineMode.h"
#include "EditorScoreData.h"
#include "HistoryManager.h"
#include <unordered_set>
#include "Audio/AudioManager.h"

#undef min
#undef max

namespace MikuMikuWorld
{
	class Renderer;
	class Texture;
	struct Score;
	struct Note;
	struct Color;
	struct Vector2;

	class ScoreEditor
	{
	private:
		TimelineMode currentMode;
		Camera camera;
		Framebuffer* framebuffer;
		AudioManager audio;
		HistoryManager history;

		Score prevUpdateScore;
		Score score;
		EditorScoreData workingData;
		std::string musicFile;
		float musicOffset;

		Note dummy;
		Note dummyStart;
		Note dummyEnd;
		Note dummyMid;
		HoldNote dummyHold;
		int hoveringNote;

		std::unordered_set<int> selectedNotes;
		std::unordered_map<int, Note> copyNotes;
		std::unordered_map<int, Note> copyNotesFlip;
		std::unordered_map<int, HoldNote> copyHolds;

		int timelineWidth = NUM_LANES * laneWidth;
		float noteCtrlHeight = (notesHeight * 0.5f);
		const float noteCtrlWidth = NOTES_SLICE_WIDTH - 2.0f;

		int division = 8;
		int selectedDivision = 1;
		int laneOffset = 0.0f;
		float laneWidth = 35.0f;
		float notesHeight = 50.0f;
		float zoom = 1.0f;
		float timelineMinOffset = 0;
		float timelineOffset = 0;
		float effectiveTickHeight;
		float cursorPos;

		int currentTick = 0;
		int hoverTick = 0;
		int hoverLane = 0;
		int pasteLane = 0;
		int holdLane = 0;
		int holdTick = 0;
		int defaultNoteWidth;
		bool windowFocused;
		bool drawHoldStepOutline;
		bool showRenderStats;
		bool isHoveringNote;
		bool isHoldingNote;
		bool isMovingNote;
		bool mouseInTimeline;
		bool dragging;
		bool insertingHold;
		bool pasting;
		bool flipPasting;
		bool hasEdit;
		bool hasSelection;
		bool hasSelectionEase;
		bool hasSelectionStep;

		ImVec2 ctrlMousePos;
		ImVec2 dragStart;
		ImVec2 mousePos;
		ImVec2 canvasSize;
		ImVec2 canvasPos;
		ImRect boundaries;

		float time;
		float timeLastFrame;
		float playStartTime;
		float songPos;
		float songPosLastFrame;
		float songStart;
		float songEnd;
		bool playing;
		float masterVolume;
		float bgmVolume;
		float seVolume;
		const float audioLookAhead = 0.1;

		void updateControls();
		void updateScoreDetails();

		void drawMeasures();
		void drawLanes();
		void updateTempoChanges();
		void updateTimeSignatures();
		void updateCursor();
		void drawSelectionRectangle();
		void drawSelectionBoxes(Renderer* renderer);
		bool noteControl(const ImVec2& pos, const ImVec2& sz, const char* id, ImGuiMouseCursor cursor);

		void updateNote(Note& note);
		void updateDummyNotes();
		void updateDummyHold();
		void drawDummyHold(Renderer* renderer);
		void cycleFlicks();
		void cycleEase();
		void setEase(EaseType ease);
		void toggleCriticals();
		void toggleStepVisibility();
		void setStepVisibility(HoldStepType type);
		void calcDragSelection();
		void debugInfo();

		int snapTickFromPos(float posY);
		int snapTick(int tick, int div);
		int roundTickDown(int tick, int div);
		int laneFromCenterPos(int lane, int width);

		void contextMenu();

		void readScoreMetadata();
		void writeScoreMetadata();

	public:
		ScoreEditor();
		~ScoreEditor();

		void update(float frameTime, Renderer* renderer);
		void updateTimeline(Renderer* renderer);

		int positionToTick(float pos, int div = 1);
		float tickToPosition(int tick, int div = 1);
		int positionToLane(float pos);
		float laneToPosition(float lane);
		float getNoteYPosFromTick(int tick);

		void updateNotes(Renderer* renderer);
		void drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldNote(const std::unordered_map<int, Note>& notes, const HoldNote& hold, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldMid(const Note& note, HoldStepType type, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHighlight(const Note& note, Renderer* renderer, const Color& tint, bool mid, const int offsetTick = 0, const int offsetLane = 0);
		bool isHoldPathInTick(const Note& n1, const Note& n2, EaseType ease, float x, float y);
		bool isNoteInCanvas(const int tick);

		void centerCursor(int mode = 0);
		void gotoMeasure(int measure);
		void changeMode(TimelineMode mode);
		void insertNote(bool critical);
		void insertNotePlaying();
		void insertHoldNote();
		void insertHoldStep(HoldNote& note);
		void insertTempo();
		void insertTimeSignature();
		int findClosestHold();

		void copy();
		void previewPaste(Renderer* renderer);
		void paste();
		void flipSelected();
		void flip();
		void flipPaste();
		void confirmPaste();
		void cancelPaste();
		void selectAll();
		void clearSelection();
		void deleteSelected();
		bool hasClipboard() const;
		bool isPasting() const;

		void togglePlaying();
		void stop();
		void restart();
		void nextTick();
		void previousTick();
		void setZoom(float val);
		void updateNoteSE();
		bool isPlaying();

		inline float getLaneWidth() { return laneWidth; }
		inline float getNotesHeight() { return notesHeight; }
		void setLaneWidth(float width);
		void setNotesHeight(float height);

		bool selectionHasEase();
		bool selectionHasHoldStep();

		void reset();
		void open();
		void save();
		void saveAs();
		void loadScore(const std::string& filename);
		void loadMusic(const std::string& filename);

		void undo();
		void redo();

		bool isWindowFocused() const;
	};
}