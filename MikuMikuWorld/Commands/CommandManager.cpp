#include "CommandManager.h"
#include "../InputListener.h"
#include "../ApplicationConfiguration.h"
#include "../ImGui/imgui.h"
#include "../UI.h"

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

	void CommandManager::writeCommands(ApplicationConfiguration& config)
	{
		config.keyConfigMap.clear();
		for (auto& command : commands)
			config.keyConfigMap[command.getName()] = KeyConfiguration{ command.getName(), {command.getKeysString()}};
	}

	void CommandManager::readCommands(const ApplicationConfiguration& config)
	{
		for (const auto& [_, keyConfig] : config.keyConfigMap)
		{
			if (keyConfig.keyBindings.size())
				setCommandKeys(keyConfig.commandName, keyConfig.keyBindings[0]);
		}
	}

	void CommandManager::updateWindow()
	{
		if (ImGui::Begin("Input Bindings"))
		{
			ImGui::Text("INPUT TIME: %f", inputTimer.elapsed());

			UI::beginPropertyColumns();
			for (int i = 0; i < commands.size(); ++i)
			{
				ImGui::Text(commands[i].getName().c_str());
				ImGui::NextColumn();

				std::string buttonText = commands[i].getKeysString();
				if (listeningForInput && editCommandIndex == i)
					buttonText = "Waiting for input...";

				if (ImGui::Button(buttonText.c_str(), ImVec2(-1, UI::btnSmall.y)))
				{
					inputTimer.reset();
					listeningForInput = true;
					editCommandIndex = i;
				}

				ImGui::NextColumn();
				ImGui::Separator();
			}

			UI::endPropertyColumns();
		}

		ImGui::End();

		if (listeningForInput)
		{
			if (inputTimer.elapsed() >= inputTimeoutSeconds)
			{
				listeningForInput = false;
				editCommandIndex = -1;
			}

			for (int i = GLFW_KEY_SPACE; i < GLFW_KEY_LAST; ++i)
			{
				bool isControl = i == GLFW_KEY_LEFT_CONTROL || i == GLFW_KEY_RIGHT_CONTROL;
				bool isShift = i == GLFW_KEY_LEFT_SHIFT || i == GLFW_KEY_RIGHT_SHIFT;
				bool isAlt = i == GLFW_KEY_LEFT_ALT || i == GLFW_KEY_RIGHT_ALT;

				if (!isControl && !isShift && !isAlt && InputListener::isTapped(i))
				{
					int mods = 0;
					mods |= CTRL & InputListener::isCtrlDown();
					mods |= ALT & InputListener::isAltDown();
					mods |= SHIFT & InputListener::isShiftDown();

					commands[editCommandIndex].setKeys(CommandKeys{ mods, i });
					listeningForInput = false;
					editCommandIndex = -1;
				}
			}
		}
	}
}