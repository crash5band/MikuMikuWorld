#pragma once
#include "Math.h"

namespace MikuMikuWorld
{
	struct WindowState
	{
		bool vsync;
		bool showPerformanceMetrics;
		bool aboutOpen;
		bool settingsOpen;
		bool unsavedOpen;
		bool maximized;
		bool closing;

		Vector2 position;
		Vector2 size;

		WindowState()
		{
			vsync = true;
			maximized = false;
			showPerformanceMetrics = false;
			aboutOpen = false;
			settingsOpen = false;
			unsavedOpen = false;
			closing = false;
		}
	};
}