#include "Command.h"
#include "CommandKeyParser.h"

namespace MikuMikuWorld
{
	Command::Command(std::string name, CommandKeys keys, Action action, Predicate predicate)
		: name{ name }, keys{ keys }, action{ action }, predicate{ predicate }
	{

	}

	std::string Command::getName() const
	{
		return name;
	}

	std::string Command::getKeysString() const
	{
		return CommandKeyParser::serialize(keys);
	}

	void Command::setKeys(CommandKeys keys)
	{
		this->keys = keys;
	}

	void Command::execute()
	{
		if (predicate())
			action();
	}

	bool Command::canExecute()
	{
		return predicate();
	}
}