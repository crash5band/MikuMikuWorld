#pragma once
#include "Commands/CommandManager.h"
#include "WindowState.h"

namespace MikuMikuWorld
{
	class Menubar
	{
	private:
		CommandManager* commandManager;
		std::unordered_map<std::string, int> commandsCache;

		void menuCommand(Command& cmd, bool selected = false);
		void openHelpPage();

	public:
		Menubar();

		void setCommandManagerInstance(CommandManager* commandManager);
		void update(WindowState& state);
	};
}