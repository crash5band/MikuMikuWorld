#pragma once
#include <random>
#include "Score.h"
#include "Math.h"
#include "EffectView.h"

namespace MikuMikuWorld
{
	struct Score;
	struct Note;
	struct ScoreContext;
}

namespace MikuMikuWorld::Engine
{
	struct DrawingNote
	{
		int refID;
		Range visualTime;
	};

	struct DrawingLine
	{
		Range xPos;
		Range visualTime;
	};

	struct DrawingHoldTick
	{
		int refID;
		float center;
		Range visualTime;
	};

	struct DrawingHoldSegment
	{
		int endID;
		EaseType ease;
		bool isGuide;
		ptrdiff_t tailStepIndex;
		double headTime, tailTime;
		float headLeft, headRight;
		float tailLeft, tailRight;
		float startTime, endTime;
		double activeTime;
	};

	struct DrawingNoteTime
	{
		int refID;
		int tick;
		int endTick;
		float time;
		float endTime;
	};

	class SortedDrawingNotesList
	{
	public:
		void add(DrawingNoteTime note);
		void add(const Note& note, const Score& score);
		void add(const HoldNote& hold, const Score& score);
		void clear();
		void reserve(size_t capacity);

		void explicitSort();

		std::vector<int> getTickRange(int from, int to) const;

		const std::vector<DrawingNoteTime>& getView() const;

	private:
		int binarySearch(int targetTick) const;
		std::vector<DrawingNoteTime> notes;
	};

	struct DrawData
	{
		float noteSpeed;
		int maxTicks;
		std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
		std::vector<DrawingHoldTick> drawingHoldTicks;
		std::vector<DrawingHoldSegment> drawingHoldSegments;

		SortedDrawingNotesList notesList;
		Effect::EffectView effectView;

		void clear();
		void calculateDrawData(Score const& score);
	};
}