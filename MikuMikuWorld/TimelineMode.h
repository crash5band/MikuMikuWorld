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
		TimelineToolMax
	};

	constexpr const char* timelineModes[]
	{
		"Select",
		"Tap",
		"Hold",
		"Hold Step",
		"Flick",
		"Critical",
		"BPM",
		"Time Signature"
	};

	constexpr int timelineModesSprites[]
	{
		0, 3, 2, 28, 1, 0, 0, 0
	};

	constexpr int divisions[]
	{
		4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, -1
	};
}