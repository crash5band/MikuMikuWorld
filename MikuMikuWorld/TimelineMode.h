#pragma once

namespace MikuMikuWorld
{
	enum class TimelineMode
	{
		Select,
		InsertTap,
		InsertLong,
		InsertLongMid,
		InsertFlick,
		MakeCritical,
		InsertBPM,
		InsertTimeSign,
		InsertHiSpeed,
		TimelineToolMax
	};

	enum class ScrollMode
	{
		None,
		Page,
		FollowCursor,
		ScrollModeMax
	};

	constexpr const char* timelineModes[]
	{
		"select",
		"tap",
		"hold",
		"hold_step",
		"flick",
		"critical",
		"bpm",
		"time_signature",
		"hi_speed"
	};

	constexpr const char* scrollModes[]
	{
		"none",
		"page",
		"follow_cursor"
	};

	constexpr int divisions[]
	{
		4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192
	};

	constexpr int timeSignatureDenominators[]
	{
		2, 4, 8, 16, 32, 64
	};
}