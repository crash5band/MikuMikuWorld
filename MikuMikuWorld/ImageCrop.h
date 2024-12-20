#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum TimelineToolSprite : uint8_t
	{
		TOOL_BPM,
		TOOL_CRITICAL,
		TOOL_FLICK_DEFAULT,
		TOOL_FLICK_LEFT,
		TOOL_FLICK_RIGHT,
		TOOL_GUIDE,
		TOOL_HI_SPEED,
		TOOL_HOLD,
		TOOL_STEP_HIDDEN,
		TOOL_STEP_NORMAL,
		TOOL_STEP_SKIP,
		TOOL_SELECT,
		TOOL_TAP,
		TOOL_TIME_SIGNATURE,
		TOOL_TRACE
	};

	enum NoteSprite : uint8_t
	{
		SPR_NOTE_CRITICAL,
		SPR_NOTE_FLICK,
		SPR_NOTE_LONG,
		SPR_NOTE_TAP,
		SPR_NOTE_FRICTION,
		SPR_NOTE_FRICTION_CRITICAL,
		SPR_NOTE_FRICTION_FLICK,
		SPR_NOTE_LONG_AMONG,
		SPR_NOTE_LONG_AMONG_CRITICAL,
		SPR_NOTE_FRICTION_AMONG,
		SPR_NOTE_FRICTION_AMONG_CRITICAL,
		SPR_NOTE_FRICTION_AMONG_FLICK,
	};

	enum FlickArrowSprite : uint8_t
	{
		SPR_FLICK_ARROW_01 = 12,
		SPR_FLICK_ARROW_01_DIAGONAL,
		SPR_FLICK_ARROW_02,
		SPR_FLICK_ARROW_02_DIAGONAL,
		SPR_FLICK_ARROW_03,
		SPR_FLICK_ARROW_03_DIAGONAL,
		SPR_FLICK_ARROW_04,
		SPR_FLICK_ARROW_04_DIAGONAL,
		SPR_FLICK_ARROW_05,
		SPR_FLICK_ARROW_05_DIAGONAL,
		SPR_FLICK_ARROW_06,
		SPR_FLICK_ARROW_06_DIAGONAL,
		SPR_FLICK_ARROW_CRITICAL_01,
		SPR_FLICK_ARROW_CRITICAL_01_DIAGONAL,
		SPR_FLICK_ARROW_CRITICAL_02,
		SPR_FLICK_ARROW_CRITICAL_02_DIAGONAL,
		SPR_FLICK_ARROW_CRITICAL_03,
		SPR_FLICK_ARROW_CRITICAL_03_DIAGONAL,
		SPR_FLICK_ARROW_CRITICAL_04,
		SPR_FLICK_ARROW_CRITICAL_04_DIAGONAL,
		SPR_FLICK_ARROW_CRITICAL_05,
		SPR_FLICK_ARROW_CRITICAL_05_DIAGONAL,
		SPR_FLICK_ARROW_CRITICAL_06,
		SPR_FLICK_ARROW_CRITICAL_06_DIAGONAL,
	};

	constexpr TimelineToolSprite timelineSprites[] =
	{
		TOOL_SELECT, TOOL_TAP, TOOL_HOLD, TOOL_STEP_NORMAL, TOOL_FLICK_DEFAULT, TOOL_CRITICAL, TOOL_TRACE, TOOL_GUIDE, TOOL_BPM, TOOL_TIME_SIGNATURE, TOOL_HI_SPEED
	};

	constexpr TimelineToolSprite flickToolSprites[] =
	{
		TOOL_FLICK_DEFAULT, TOOL_FLICK_LEFT, TOOL_FLICK_RIGHT
	};

	constexpr TimelineToolSprite stepToolSprites[] =
	{
		TOOL_STEP_NORMAL, TOOL_STEP_HIDDEN, TOOL_STEP_SKIP
	};
}