#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum class NoteType : uint8_t
	{
		Tap,
		Hold,
		HoldMid,
		HoldEnd,
	};

	enum class FlickType : uint8_t
	{
		None,
		Up,
		Left,
		Right,
		FlickTypeCount
	};

	constexpr const char* flickTypes[]
	{
		"none",
		"up",
		"left",
		"right"
	};

	enum class HoldStepType : uint8_t
	{
		Visible,
		Invisible,
		Ignored,
		HoldStepTypeCount
	};

	constexpr const char* stepTypes[]
	{
		"visible",
		"invisible",
		"ignored"
	};

	enum class EaseType : uint8_t
	{
		None,
		EaseIn,
		EaseOut,
		EaseTypeCount
	};

	constexpr const char* easeTypes[]
	{
		"none",
		"in",
		"out"
	};
}