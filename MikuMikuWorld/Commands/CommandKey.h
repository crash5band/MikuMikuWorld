#pragma once
#include <vector>

namespace MikuMikuWorld
{
	enum KeyModifiers
	{
		NONE	= 0x0,
		CTRL	= 0x1,
		SHIFT	= 0x2,
		ALT		= 0x4
	};

	struct CommandKeys
	{
		int modifiers;
		int keyCode;
	};
}