#pragma once
#include <cmath>
#include <array>
#include "Score.h"
#include "Math.h"

namespace MikuMikuWorld
{
    namespace Engine
    {
        enum class SpriteType: int {
			NoteLeft,
			NoteMiddle,
			NoteRight,
            FlickArrowLeft,
            FlickArrowUp = FlickArrowLeft + 6
		};

        struct Range {
			float min;
			float max;
		};

        Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed);

        static inline float approach(float start_time, float end_time, float current_time) {
			return std::pow(1.06, 45 * lerp(-1, 0, unlerp(start_time, end_time, current_time)));
		}
        static inline float getStageStart(float height) { return height * (0.5 + 1.15875 * (47.f / 1176)); }
        static inline float getStageEnd(float height) { return height * (0.5 - 1.15875 * (803. / 1176)); }
        static inline float getLaneWidth(float width) { return width * ((1.15875 * (1420. / 1176)) / (16. / 9) / 12.); }
        static inline float laneToLeft(float lane) { return lane - 6; }
    } // namespace Engine
}