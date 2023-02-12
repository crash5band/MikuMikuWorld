#pragma once
#include "Command.h"
#include "CommandKeyParser.h"

namespace MikuMikuWorld
{
	class CommandManager
	{
	private:
		std::vector<Command> commands;

	public:
		CommandManager();

		void add(std::string name, CommandKeys keys, Action action, Predicate predicate = [] { return true; });
		void execute(std::string name);
		bool canExecute(std::string name);
		bool testCommand(Command cmd, int mods);
		void processKeyboardShortcuts();

		bool setCommandKeys(std::string cmd, std::string keys);
		std::string getCommandKeys(std::string cmd);
	};
}