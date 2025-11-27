#pragma once
#include <random>
#include <array>
#include <set>
#include "Score.h"
#include "Math.h"
#include "EffectView.h"
#include <memory>

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

	struct DrawData
	{
		float noteSpeed;
		int maxTicks;
		std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
		std::vector<DrawingHoldTick> drawingHoldTicks;
		std::vector<DrawingHoldSegment> drawingHoldSegments;
		Effect::EffectView effectView;

		void clear();
		void calculateDrawData(Score const& score);
	};
}