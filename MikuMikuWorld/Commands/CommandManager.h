#pragma once
#include "Command.h"
#include "CommandKeyParser.h"
#include "../Stopwatch.h"

namespace MikuMikuWorld
{
	class ApplicationConfiguration;

	class CommandManager
	{
	private:
		std::vector<Command> commands;
		Stopwatch inputTimer;

		const int inputTimeoutSeconds = 5;
		bool listeningForInput = false;
		int editCommandIndex = -1;

	public:
		CommandManager();

		void add(std::string name, CommandKeys keys, Action action, Predicate predicate = [] { return true; });
		void execute(std::string name);
		bool canExecute(std::string name);
		bool testCommand(Command cmd, int mods);
		void processKeyboardShortcuts();

		bool setCommandKeys(std::string cmd, std::string keys);
		std::string getCommandKeys(std::string cmd);

		void writeCommands(ApplicationConfiguration& config);
		void readCommands(const ApplicationConfiguration& config);
		
		void updateWindow();
	};
}