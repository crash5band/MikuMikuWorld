#pragma once
#include "Commands/CommandManager.h"

namespace MikuMikuWorld
{
	class Menubar
	{
	private:
		CommandManager* commandManager;
		std::unordered_map<std::string, int> commandsCache;

		void menuItem(std::string label, Command& cmd, bool selected = false);

	public:
		Menubar();

		void setCommandManagerInstance(CommandManager* commandManager);
		void update(bool& settingsOpen, bool& aboutOpen, bool& vsync, bool& exiting, bool& showPerformanceMetrics);
	};
}