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
		Right
	};

	enum class HoldStepType : uint8_t
	{
		Visible,
		Invisible,
		Ignored
	};

	enum class EaseType : uint8_t
	{
		None,
		EaseIn,
		EaseOut
	};
}