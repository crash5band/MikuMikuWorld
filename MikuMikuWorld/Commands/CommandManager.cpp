#include "CommandManager.h"
#include "../InputListener.h"

namespace MikuMikuWorld
{
	CommandManager::CommandManager()
	{

	}

	void CommandManager::add(std::string name, CommandKeys keys, Action action, Predicate predicate)
	{
		commands.push_back(Command(name, keys, action, predicate));
	}

	bool CommandManager::canExecute(std::string name)
	{
		for (auto& command : commands)
			if (command.getName() == name)
				return command.canExecute();

		return false;
	}

	void CommandManager::execute(std::string name)
	{
		for (auto& command : commands)
			if (command.getName() == name)
				command.execute();
	}

	void CommandManager::processKeyboardShortcuts()
	{
		int mods = 0;
		if (InputListener::isCtrlDown())
			mods |= CTRL;

		if (InputListener::isShiftDown())
			mods |= SHIFT;

		if (InputListener::isAltDown())
			mods |= ALT;

		for (auto& command : commands)
		{
			if (testCommand(command, mods))
				command.execute();
		}
	}

	bool CommandManager::testCommand(Command cmd, int mods)
	{
		if (mods != cmd.getMods() || cmd.getKey() == 0)
			return false;

		return InputListener::isTapped(cmd.getKey());
	}

	std::string CommandManager::getCommandKeys(std::string cmd)
	{
		for (auto& command : commands)
			if (command.getName() == cmd)
				return command.getKeysString();

		return "";
	}

	bool CommandManager::setCommandKeys(std::string cmd, std::string keys)
	{
		for (auto& command : commands)
		{
			if (command.getName() == cmd)
			{
				command.setKeys(CommandKeyParser::deserialize(keys));
				return true;
			}
		}

		return false;
	}
}