#pragma once
#include <random>
#include <array>
#include "Score.h"
#include "Math.h"

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
        int headID, tailID;
        float headTime;
        float tailTime;
        float startTime;
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
        int refID;
        Range time;
        std::array<DrawingParticleProperty, 6> xywhta;
    };

    struct DrawData
    {
        float noteSpeed;
        int maxTicks;
        std::default_random_engine rng;
        std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
        std::vector<DrawingHoldTick> drawingHoldTicks;
        std::vector<DrawingHoldSegment> drawingHoldSegments;
        std::vector<DrawingParticle> drawingParticles;
        bool hasLaneEffect;
        bool hasNoteEffect;

        void clear();
        void calculateDrawData(Score const& score);
    };
}