#pragma once
#include <random>
#include <array>
#include <list>
#include "Score.h"
#include "Math.h"

namespace MikuMikuWorld
{
	struct Score;
	struct Note;
	struct ScoreContext;
}

namespace MikuMikuWorld::Engine
{
	enum class DrawingParticleType : uint8_t
	{
		Lane,
		Circular,
		BlendCircular,
		Linear,
		Flat,
		Slot,
		SlotGlow,
		SlotGlowAdditive
	};

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
		float headTime, headLeft, headRight;
		float tailTime, tailLeft, tailRight;
		float startTime, endTime;
		float activeTime;
		EaseType ease;
		bool isGuide;
	};

	struct DrawingParticleProperty
	{
		Range value;
		EasingFunc ease;
		inline float at(float p) const { return lerp(value.min, value.max, this->ease(p)); }
	};

	struct DrawingParticle
	{
		int effectType;
		int particleId;
		Range time;
		std::array<DrawingParticleProperty, 6> xywhta;

		DrawingParticleType type;
	};

	struct DrawingEffect
	{
		int refID;
		Range time;

		std::vector<DrawingParticle> additiveAlphaBlendParticles;
		std::vector<DrawingParticle> blendParticles;
		std::vector<DrawingParticle> laneParticles;
		std::vector<DrawingParticle> additiveParticles;
		std::unique_ptr<DrawingParticle> slotParticle;
	};

	struct DrawData
	{
		float noteSpeed;
		int maxTicks;
		std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
		std::vector<DrawingHoldTick> drawingHoldTicks;
		std::vector<DrawingHoldSegment> drawingHoldSegments;
		std::map<int, DrawingEffect> drawingEffects;
		bool hasLaneEffect;
		bool hasNoteEffect;
		bool hasSlotEffect;

		void clear();
		void calculateDrawData(Score const& score);
		void updateNoteEffects(ScoreContext& context);
	};
}