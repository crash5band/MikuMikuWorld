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

    struct PreviewConfiguration {
        float noteSpeed = 6;
    };

    struct DrawData {
        PreviewConfiguration config;
        std::vector<DrawingNote> drawingNotes;
		std::vector<DrawingLine> drawingLines;

        void clear();
        void calculateDrawData(Score const& score);
    };

    Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed);

    /// General helper functions for fixed values in the engine
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
    static inline float getNoteHeight() {
        return 75. / 850. / 2.;
    }
    static inline int getZIndex(Layer layer, float xOffset, float yOffset) {
        static_assert(sizeof(int) == sizeof(int32_t));
        int32_t hhight = static_cast<int32_t>(layer) << 24;
        yOffset = 1 - yOffset;
        int32_t mid = static_cast<int32_t>((yOffset < 0.f ? 0.f : (yOffset <= 1.f ? yOffset : 1.f)) * float(UINT16_MAX) + 0.5f) << 8;
        xOffset = (xOffset / 12.f) + 0.5f;
        int32_t llow = static_cast<int32_t>((xOffset < 0.f ? 0.f : (xOffset < 1.f ? xOffset : 1.f)) * float(UINT8_MAX) + 0.5f);
        return INT32_MAX - hhight - mid - llow;
    }
}