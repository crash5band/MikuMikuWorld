#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace MikuMikuWorld
{
	struct CommandKeys;
	enum KeyModifiers;

	class CommandKeyParser
	{
	private:
		static std::unordered_map<std::string, KeyModifiers> modifierNames;

	public:
		static CommandKeys deserialize(std::string keys);
		static std::string serialize(CommandKeys keys);

		static int keyCharToCode(std::string key);
		static std::string keyCodeToChar(int code);

		static KeyModifiers getModifiersFromCode(int code);
		static KeyModifiers getModifiersFromKey(std::string key);
	};
}