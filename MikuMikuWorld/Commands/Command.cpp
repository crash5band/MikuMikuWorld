#include "Command.h"
#include "CommandKeyParser.h"
#include "../Localization.h"

namespace MikuMikuWorld
{
	Command::Command(std::string name, std::vector<CommandKeys> keys, Action action, Predicate predicate)
		: name{ name }, keys { keys }, action{ action }, predicate{ predicate }
	{

	}

	std::string Command::getName() const
	{
		return name;
	}

	std::string Command::getDisplayName() const
	{
		std::string localizedString =  getString(name);
		return localizedString == " " ? name : localizedString;
	}

	std::string Command::getKeysString(int index) const
	{
		if (index < 0 || index >= keys.size())
			return "";

		return CommandKeyParser::serialize(keys[index]);
	}

	std::string Command::getAllKeysString() const
	{
		std::string result;
		for (int i = 0; i < keys.size(); ++i)
		{
			result += CommandKeyParser::serialize(keys[i]);
			if (i < keys.size() - 1)
				result += ", ";
		}
		
		return result;
	}

	void Command::setKeys(int index, CommandKeys keys)
	{
		if (index < 0 || index >= this->keys.size())
			return;

		this->keys[index] = keys;
	}

	void Command::addKeys(CommandKeys keys)
	{
		this->keys.push_back(keys);
	}

	void Command::removeKeys(int index)
	{
		if (index < 0 || index >= this->keys.size())
			return;

		this->keys.erase(this->keys.begin() + index);
	}

	int Command::getBindingsCount() const
	{
		return this->keys.size();
	}

	int Command::getMods(int index) const
	{
		if (index < 0 || index >= keys.size())
			return 0;

		return keys[index].modifiers;
	}

	int Command::getKey(int index) const
	{
		if (index < 0 || index >= keys.size())
			return 0;

		return keys[index].keyCode;
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