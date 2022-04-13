#pragma once
#include <stdint.h>

namespace MikuMikuWorld
{
	enum class AnchorType : uint8_t
	{
		TopLeft,
		TopCenter,
		TopRight,
		MiddleLeft,
		MiddleCenter,
		MiddleRight,
		BottomLeft,
		BottomCenter,
		BottomRight
	};
}