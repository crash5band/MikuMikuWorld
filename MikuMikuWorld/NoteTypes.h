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
		Default,
		Left,
		Right,
		FlickTypeCount
	};

	constexpr const char* flickTypes[]
	{
		"none",
		"default",
		"left",
		"right"
	};

	enum class HoldStepType : uint8_t
	{
		Normal,
		Hidden,
		Skip,
		HoldStepTypeCount
	};

	constexpr const char* stepTypes[]
	{
		"normal",
		"hidden",
		"skip"
	};

	enum class EaseType : uint8_t
	{
		Linear,
		EaseIn,
		EaseOut,
		EaseTypeCount
	};

	constexpr const char* easeTypes[]
	{
		"linear",
		"ease_in",
		"ease_out"
	};

	enum class HoldNoteType : uint8_t
	{
		Normal,
		Hidden,
		Guide
	};

	constexpr const char* holdTypes[]
	{
		"normal",
		"hidden",
		"guide"
	};
}