#pragma once
#include <string>
#include <vector>
#include <functional>
#include "CommandKey.h"

namespace MikuMikuWorld
{
	using Predicate = std::function<bool()>;
	using Action = std::function<void()>;

	class Command
	{
	private:
		std::string name;
		std::vector<CommandKeys> keys;

		Action action;
		Predicate predicate;

	public:
		Command(std::string name, std::vector<CommandKeys> keys, Action action, Predicate predicate = []{ return true; });

		std::string getName() const;
		std::string getDisplayName() const;

		std::string getKeysString(int index) const;
		std::string getAllKeysString() const;

		void setKeys(int index, CommandKeys keys);
		void addKeys(CommandKeys keys);
		void removeKeys(int index);

		bool canExecute();
		void execute();

		int getMods(int index) const;
		int getKey(int index) const;
		int getBindingsCount() const;
	};
}