#pragma once
#include "Score.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "Rendering/Camera.h"
#include "Rendering/Framebuffer.h"
#include "Constants.h"
#include "TimelineMode.h"
#include "EditorScoreData.h"
#include "HistoryManager.h"
#include "Audio/AudioManager.h"
#include "PresetManager.h"
#include "ScoreStats.h"
#include "Canvas.h"
#include <unordered_set>

// needed for miniaudio to work
#undef min
#undef max

namespace MikuMikuWorld
{
	class Renderer;
	struct Score;
	struct Note;
	struct Color;

	class ScoreEditor
	{
	private:
		TimelineMode currentMode = TimelineMode::Select;
		ScrollMode scrollMode = ScrollMode::Smooth;
		Camera camera;
		Framebuffer* framebuffer;
		ScoreStats stats;
		ImGuiTextFilter presetFilter;

		Score prevUpdateScore;
		Score score;
		EditorScoreData workingData;
		std::string musicFile;
		float musicOffset;
		std::unordered_map<std::string, int> tickSEMap;

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

		std::unordered_map<int, Note> presetNotes;
		std::unordered_map<int, HoldNote> presetHolds;

		float noteCtrlHeight;
		const float noteCtrlWidth = NOTES_SLICE_WIDTH - 2.0f;

		int division = 8;
		int customDivision = 8;
		int selectedDivision = 1;

		int currentTick = 0;
		int hoverTick = 0;
		int hoverLane = 0;
		int pasteLane = 0;
		int holdLane = 0;
		int holdTick = 0;

		int defaultNoteWidth;
		HoldStepType defaultStepType;
		float defaultBPM;
		int defaultTimeSignN;
		int defaultTimeSignD;

		bool windowFocused;
		bool drawHoldStepOutline;
		bool showRenderStats;
		bool isHoveringNote;
		bool isHoldingNote;
		bool isMovingNote;
		bool skipUpdateAfterSortingSteps;
		bool dragging;
		bool insertingHold;
		bool pasting;
		bool flipPasting;
		bool insertingPreset;
		bool hasEdit;
		bool hasSelection;
		bool hasSelectionEase;
		bool hasSelectionStep;
		bool hasFlickable;
		bool uptoDate;

		ImVec2 ctrlMousePos;
		ImVec2 dragStart;
		ImVec2 mousePos;

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
		const float audioLookAhead = 0.05f;

		// update methods
		void updateNotes(Renderer* renderer);
		void updateNote(Note& note);
		void updateDummyNotes();
		void updateDummyHold();
		void updateTempoChanges();
		void updateTimeSignatures();
		void updateCursor();
		void updateTimeline(float frameTime, Renderer* renderer);
		void updateToolboxWindow();
		void updatePresetsWindow();

		// edit methods
		void cycleFlicks();
		void setFlick(FlickType flick);
		void cycleEase();
		void setEase(EaseType ease);
		void toggleCriticals();
		void cycleStepType();
		void setStepType(HoldStepType type);
		void pushHistory(const std::string& description, const Score& prev, const Score& curr);

		// draw methods
		void drawMeasures();
		void drawLanes();
		void drawNote(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawFlickArrow(const Note& note, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldNote(const std::unordered_map<int, Note>& notes, const HoldNote& hold, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldMid(const Note& note, HoldStepType type, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHoldCurve(const Note& n1, const Note& n2, EaseType ease, Renderer* renderer, const Color& tint, const int offsetTick = 0, const int offsetLane = 0);
		void drawHighlight(const Note& note, Renderer* renderer, const Color& tint, bool mid, const int offsetTick = 0, const int offsetLane = 0);
		void drawDummyHold(Renderer* renderer);
		void drawSelectionRectangle();
		void drawSelectionBoxes(Renderer* renderer);

		// helper methods
		std::string getDivisonString(int divIndex);
		int snapTickFromPos(float posY);
		int snapTick(int tick, int div);
		int roundTickDown(int tick, int div);
		int laneFromCenterPos(int lane, int width);
		int findClosestHold();
		void calcDragSelection();
		bool noteControl(const ImVec2& pos, const ImVec2& sz, const char* id, ImGuiMouseCursor cursor);
		bool isHoldPathInTick(const Note& n1, const Note& n2, EaseType ease, float x, float y);

		// editor window update methods
		void updateControls();
		void updateScoreDetails();
		void contextMenu();
		void debugInfo();

		void readScoreMetadata();
		void writeScoreMetadata();
		void resetEditor();

	public:
		Canvas canvas;
		AudioManager audio;
		HistoryManager history;
		PresetManager presetManager;

		ScoreEditor();
		~ScoreEditor();

		// main update method
		void update(float frameTime, Renderer* renderer);

		// edit methods
		void gotoMeasure(int measure);
		void changeMode(TimelineMode mode);
		void insertNote(bool critical);
		void insertNotePlaying();
		void insertHoldNote();
		void insertHoldStep(HoldNote& note);
		void insertTempo();
		void insertTimeSignature();
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

		// playback methods
		void togglePlaying();
		void stop();
		void restart();
		void nextTick();
		void previousTick();
		void updateNoteSE();
		bool isPlaying();

		inline int getDivision() { return division; }
		void setDivision(int div);
		bool selectionHasEase();
		bool selectionHasHoldStep();
		bool selectionHasFlickable();

		// IO methods
		void reset();
		void open();
		void save();
		void save(const std::string& filename);
		void saveAs();
		void exportSUS();
		void loadScore(const std::string& filename);
		void loadMusic(const std::string& filename);
		bool isUptoDate() const;
		std::string getWorkingFilename() const;

		// history methods
		void undo();
		void redo();

		bool isWindowFocused() const;
	};
}
