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
		CommandKeys keys;

		Action action;
		Predicate predicate;

	public:
		Command(std::string name, CommandKeys keys, Action action, Predicate predicate = []{ return true; });

		std::string getName() const;

		std::string getKeysString() const;
		void setKeys(CommandKeys keys);

		bool canExecute();
		void execute();

		inline int getMods() const { return keys.modifiers; }
		inline int getKey() const { return keys.keyCode; }
	};
}