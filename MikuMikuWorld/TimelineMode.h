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
		MakeFriction,
		InsertBPM,
		InsertTimeSign,
		InsertHiSpeed,
		TimelineModeMax
	};

	constexpr const char* timelineModes[]
	{
		"select",
		"tap",
		"hold",
		"hold_step",
		"flick",
		"critical",
		"trace",
		"bpm",
		"time_signature",
		"hi_speed"
	};

	constexpr int divisions[]
	{
		4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192
	};

	constexpr int timeSignatureDenominators[]
	{
		2, 4, 8, 16, 32, 64
	};

	enum class EventType
	{
		None = -1,
		Bpm,
		TimeSignature,
		HiSpeed,
		Skill,
		Fever,
		EventTypeMax
	};

	constexpr const char* eventTypes[]
	{
		"bpm",
		"time_signature",
		"hi_speed",
		"skill",
		"fever"
	};

	enum class Direction
	{
		Down,
		Up,
		DirectionMax
	};
}