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

	constexpr const char* flickTypes[]{ "none", "default", "left", "right" };

	enum class HoldStepType : uint8_t
	{
		Normal,
		Hidden,
		Skip,
		HoldStepTypeCount
	};

	constexpr const char* stepTypes[]{ "normal", "hidden", "skip" };

	enum class EaseType : uint8_t
	{
		Linear,
		EaseIn,
		EaseOut,
		EaseInOut,
		EaseOutIn,
		EaseTypeCount
	};

	constexpr const char* easeNames[]{ "linear", "in", "out", "inout", "outin" };

	constexpr const char* easeTypes[]{ "linear", "ease_in", "ease_out", "ease_in_out",
		                               "ease_out_in" };

	enum class HoldNoteType : uint8_t
	{
		Normal,
		Hidden,
		Guide
	};

	constexpr const char* holdTypes[]{ "normal", "hidden", "guide" };

	enum class GuideColor : uint8_t
	{
		Neutral,
		Red,
		Green,
		Blue,
		Yellow,
		Purple,
		Cyan,
		Black,
		GuideColorCount
	};

	constexpr const char* guideColors[]{ "neutral", "red",    "green", "blue",
		                                 "yellow",  "purple", "cyan",  "black" };
	constexpr const char* guideColorsForString[]{ "guide_neutral", "guide_red",    "guide_green",
		                                          "guide_blue",    "guide_yellow", "guide_purple",
		                                          "guide_cyan",    "guide_black" };

	enum class FadeType : uint8_t
	{
		Out,
		None,
		In
	};

	constexpr const char* fadeTypes[]{ "fade_out", "fade_none", "fade_in" };
}
