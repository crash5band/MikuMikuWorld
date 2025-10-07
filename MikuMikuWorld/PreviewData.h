#pragma once
#include <random>
#include <array>
#include <set>
#include "Score.h"
#include "Math.h"
#include <memory>

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
		Linear,
		Flat,
		Slot,
		Aura
	};

	enum class NoteEffectType : uint8_t
	{
		Lane,
		Slot,
		Gen,
		GenHold,
		Aura,
		AuraHold,
		NoteEffectTypeCount
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
		EaseType ease;
		bool isGuide;
		ptrdiff_t tailStepIndex;
		double headTime, tailTime;
		float headLeft, headRight;
		float tailLeft, tailRight;
		float startTime, endTime;
		double activeTime;
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

		// u1 and u2 are "user data" fields
		// Currently u1 is used as an "elevation" parameter for circular effects
		std::array<DrawingParticleProperty, 8> xywhtau1u2;

		DrawingParticleType type;
	};

	struct DrawingEffect
	{
		int refID{};
		int lane{};
		Range time{-1, -1};

		std::vector<DrawingParticle> particles;
	};

	struct EffectPool
	{
		size_t nextIndex{};
		std::array<DrawingEffect, 12> pool;

		DrawingEffect& getNext(int noteId, bool advance);
		void clear();
		int findIndex(int noteId, int lane);
	};

	struct DrawData
	{
		float noteSpeed;
		int maxTicks;
		std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
		std::vector<DrawingHoldTick> drawingHoldTicks;
		std::vector<DrawingHoldSegment> drawingHoldSegments;

		std::map<NoteEffectType, EffectPool> normalEffectsPools;
		std::map<NoteEffectType, EffectPool> criticalEffectsPools;
		std::set<int> drawingNoteEffects;

		bool hasLaneEffect;
		bool hasNoteEffect;
		bool hasSlotEffect;

		void clear();
		void clearEffectPools();
		void calculateDrawData(Score const& score);
		void updateNoteEffects(ScoreContext& context);
		void initEffectPools();
	};
}