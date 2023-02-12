#include "../StringOperations.h"
#include "../InputListener.h"
#include "../KeyNames.h"
#include "CommandKeyParser.h"
#include "CommandKey.h"
#include <vector>
#include <string>
#include <algorithm>

namespace MikuMikuWorld
{
	std::unordered_map<std::string, KeyModifiers> CommandKeyParser::modifierNames = {
		{ "ctrl", CTRL },
		{ "alt", ALT },
		{ "shift", SHIFT }
	};

	int CommandKeyParser::keyCharToCode(std::string key)
	{
		key = trim(key);
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);

		for (int i = 0; i < sizeof(printableKeyNames) / sizeof(const char*); ++i)
		{
			std::string name = printableKeyNames[i];
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);

			if (name == key)
				return i + printableKeysOffset;
		}

		for (int i = 0; i < sizeof(functionKeyNames) / sizeof(const char*); ++i)
		{
			std::string name = functionKeyNames[i];
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);

			if (name == key)
				return i + functionKeysOffset;
		}

		return 0;
	}

	std::string CommandKeyParser::keyCodeToChar(int code)
	{
		if (code >= printableKeysOffset && code <= (sizeof(printableKeyNames) / sizeof(const char*)) + printableKeysOffset)
			return printableKeyNames[code - printableKeysOffset];

		if (code >= functionKeysOffset && code <= (sizeof(functionKeyNames) / sizeof(const char*)) + functionKeysOffset)
			return functionKeyNames[code - functionKeysOffset];

		return "";
	}

	KeyModifiers CommandKeyParser::getModifiersFromCode(int code)
	{
		switch (code)
		{
		case GLFW_KEY_LEFT_CONTROL:
		case GLFW_KEY_RIGHT_CONTROL:
			return CTRL;

		case GLFW_KEY_LEFT_ALT:
		case GLFW_KEY_RIGHT_ALT:
			return ALT;

		case GLFW_KEY_LEFT_SHIFT:
		case GLFW_KEY_RIGHT_SHIFT:
			return SHIFT;

		default:
			return NONE;
		}
	}

	KeyModifiers CommandKeyParser::getModifiersFromKey(std::string key)
	{
		key = trim(key);
		std::transform(key.begin(), key.end(), key.begin(), ::tolower);

		return modifierNames.find(key) == modifierNames.end() ? NONE : modifierNames[key];
	}

	CommandKeys CommandKeyParser::deserialize(std::string keys)
	{
		std::vector<std::string> splitKeys = split(keys, " ");
		int mods = NONE;

		for (int i = 0; i < splitKeys.size(); i += 2)
		{
			std::string name = trim(splitKeys[i]);
			std::transform(name.begin(), name.end(), name.begin(), ::tolower);

			for (const auto& [key, value] : modifierNames)
			{
				if (name == key && !(mods & value))
					mods |= value;
			}
		}

		return CommandKeys{ mods, keyCharToCode(splitKeys[splitKeys.size() - 1])};
	}

	std::string CommandKeyParser::serialize(CommandKeys keys)
	{
		std::string keyString = "";
		if (keys.modifiers & CTRL)
			keyString += "Ctrl + ";

		if (keys.modifiers & SHIFT)
			keyString += "Shift + ";

		if (keys.modifiers & ALT)
			keyString += "Alt + ";

		keyString += keyCodeToChar(keys.keyCode);
		return keyString;
	}
}