#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum class TimelineMode : uint8_t
	{
		Select,
		InsertTap,
		InsertLong,
		InsertLongMid,
		InsertFlick,
		MakeCritical,
		MakeFriction,
		InsertGuide,
		InsertDamage,
		InsertBPM,
		InsertTimeSign,
		InsertHiSpeed,
		TimelineModeMax
	};

	constexpr const char* timelineModes[]{ "select", "tap",      "hold",           "hold_step",
		                                   "flick",  "critical", "trace",          "guide",
		                                   "damage", "bpm",      "time_signature", "hi_speed" };

	constexpr int divisions[]{ 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192 };

	enum class SnapMode : uint8_t
	{
		Relative,
		Absolute,
		SnapModeMax
	};

	constexpr const char* snapModes[]{ "snap_mode_relative", "snap_mode_absolute" };

	constexpr int timeSignatureDenominators[]{ 2, 4, 8, 16, 32, 64 };

	enum class EventType : uint8_t
	{
		None,
		Bpm,
		TimeSignature,
		HiSpeed,
		Skill,
		Fever,
		Waypoint,
		EventTypeMax
	};

	constexpr const char* eventTypes[]{ "none",  "bpm",   "time_signature", "hi_speed",
		                                "skill", "fever", "waypoint" };

	enum class Direction : uint8_t
	{
		Down,
		Up,
		DirectionMax
	};
}
