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
    Damage,
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

	enum class GuideColor : uint8_t
	{
    Neutral,
    Red,
    Green,
    Blue,
    Yellow,
    Purple,
    Cyan,
	};

  constexpr const char* guideColors[]
  {
    "neutral",
    "red",
    "green",
    "blue",
    "yellow",
    "purple",
    "cyan"
  };

	enum class FadeType : uint8_t
	{
    Out,
    None,
    In
	};

	constexpr const char* fadeTypes[]
	{
		"fade_out",
		"fade_none",
		"fade_in"
	};
}
