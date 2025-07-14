#pragma once
#include <cmath>
#include <array>
#include "Score.h"
#include "Math.h"

namespace MikuMikuWorld::Engine
{

    enum class SpriteType: int {
        NoteLeft,
        NoteMiddle,
        NoteRight,
        TraceDiamond,
        FlickArrowLeft,
        FlickArrowUp = FlickArrowLeft + 6,
        SimultaneousLine = FlickArrowUp + 6,
        HoldTick
    };

    enum class Layer : uint8_t {
        FLICK_ARROW,
        DIAMOND,
        BASE_NOTE,
        TICK_NOTE,
        HOLD_MID,
        HOLD_PATH
        
    };

    struct Range {
        float min;
        float max;
    };

    struct DrawingNote {
        int refID;
        Range visualTime;
    };

    struct DrawingLine {
        Range xPos;
        Range visualTime;
    };

    struct DrawingHoldTick {
        int refID;
        float center;
        Range visualTime;
    };

    struct DrawingHoldSegment {
        int endID;
        int headID, tailID;
        float headTime;
        float tailTime;
        float startTime;
        EaseType ease;
        bool isGuide;
    };

    struct DrawData {
        float noteSpeed;
        int maxTicks;
        std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;
        std::vector<DrawingHoldTick> drawingHoldTicks;
        std::vector<DrawingHoldSegment> drawingHoldSegments;

        void clear();
        void calculateDrawData(Score const& score);
    };

    Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed);

    /// General helper functions for fixed values in the engine
    static inline float getNoteDuration(float noteSpeed) {
        return lerp(0.35, 4.f, std::pow(unlerp(12, 1, noteSpeed), 1.31f));
    }
    static inline float approach(float start_time, float end_time, float current_time) {
        return std::pow(1.06, 45 * lerp(-1, 0, unlerp(start_time, end_time, current_time)));
    }
    static inline float getStageStart(float height) {
        return height * (0.5 + 1.15875 * (47.f / 1176));
    }
    static inline float getStageEnd(float height) {
        return height * (0.5 - 1.15875 * (803. / 1176));
    }
    static inline float getLaneWidth(float width) {
        return width * ((1.15875 * (1420. / 1176)) / (16. / 9) / 12.);
    }
    static inline float laneToLeft(float lane) {
        return lane - 6;
    }
    static inline float getNoteCenter(Note const& note) {
        return laneToLeft(note.lane) + note.width / 2.f; 
    }
    static inline float getNoteHeight() {
        return 75. / 850. / 2.;
    }
    static inline int getZIndex(Layer layer, float xOffset, float yOffset) {
        static_assert(sizeof(int) == sizeof(int32_t));
        // Implicitly clamp NaN to max value unlike normal clamp
        const auto floatClamp = [](float value, float min, float max) { return value < min ? min : (value <= max ? value : max); };
        const auto masknShift = [](int32_t value, int32_t mask, int offset) { return (value & mask) << offset; };
        const int32_t mask24 = 0xFFFFFF, mask4 = 0x0F;
        int32_t y = static_cast<int32_t>(floatClamp(1 - yOffset, 0, 1) * float(mask24) + 0.5f);
        int32_t x = static_cast<int32_t>(floatClamp(xOffset / 12.f + 0.5f, 0, 1) * float(12) + 0.5f);
        return INT32_MAX
            - masknShift(static_cast<int32_t>(layer), mask4, 32 - 4)
            - masknShift(y, mask24, 4)
            - masknShift(x, mask4, 0);
    }
}