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
		Stopwatch inputTimer;

		const int inputTimeoutSeconds = 5;
		bool listeningForInput = false;
		int editBindingIndex = -1;
		int selectedCommandIndex = -1;

	public:
		std::vector<Command> commands;

		CommandManager();

		void add(std::string name, std::vector<CommandKeys> keys, Action action, Predicate predicate = [] { return true; });
		void execute(std::string name);
		bool canExecute(std::string name);
		bool testCommand(Command cmd, int mods);
		void processKeyboardShortcuts();

		int findCommand(std::string name);

		bool setCommandKeys(int cmd, int binding, std::string keys);
		void addCommandKeys(int index, std::string keys);
		void removeCommandKeys(int index);
		std::string getCommandKeys(std::string cmd, int index);

		void writeCommands(ApplicationConfiguration& config);
		void readCommands(const ApplicationConfiguration& config);
		
		void updateWindow();
	};
}