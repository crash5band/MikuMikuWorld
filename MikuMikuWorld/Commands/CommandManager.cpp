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

	void CommandManager::add(std::string name, std::vector<CommandKeys> keys, Action action, Predicate predicate)
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

	int CommandManager::findCommand(std::string name)
	{
		for (int i = 0; i < commands.size(); ++i)
		{
			if (commands[i].getName() == name)
				return i;
		}

		return -1;
	}

	bool CommandManager::testCommand(Command cmd, int mods)
	{
		for (int i = 0; i < cmd.getBindingsCount(); ++i)
		{
			if (mods != cmd.getMods(i) || cmd.getKey(i) == 0)
				continue;
			
			if (InputListener::isTapped(cmd.getKey(i)))
				return true;
		}

		return false;
	}

	std::string CommandManager::getCommandKeys(std::string cmd, int index)
	{
		for (auto& command : commands)
			if (command.getName() == cmd)
				return command.getKeysString(index);

		return "";
	}

	bool CommandManager::setCommandKeys(int cmdIndex, int keysIndex, std::string keys)
	{
		if (cmdIndex < 0 || cmdIndex >= commands.size())
			return false;

		commands[cmdIndex].setKeys(keysIndex, CommandKeyParser::deserialize(keys));
		return true;
	}

	void CommandManager::addCommandKeys(int index, std::string keys)
	{
		if (index < 0 || index >= commands.size())
			return;

		commands[index].addKeys(CommandKeyParser::deserialize(keys));
	}

	void CommandManager::removeCommandKeys(int index)
	{
		if (index < 0 || index >= commands.size())
			return;

		commands.erase(commands.begin() + index);
	}

	void CommandManager::writeCommands(ApplicationConfiguration& config)
	{
		config.keyConfigMap.clear();
		for (auto& command : commands)
		{
			std::vector<std::string> bindings;
			for (int i = 0; i < command.getBindingsCount(); ++i)
				bindings.push_back(command.getKeysString(i));

			config.keyConfigMap[command.getName()] = KeyConfiguration{ command.getName(), bindings };
		}
	}

	void CommandManager::readCommands(const ApplicationConfiguration& config)
	{
		for (const auto& [_, keyConfig] : config.keyConfigMap)
		{
			int cmdIndex = findCommand(keyConfig.commandName);
			if (cmdIndex == -1)
				continue;

			for (int i = 0; i < keyConfig.keyBindings.size(); ++i)
			{
				if (i >= commands[cmdIndex].getBindingsCount())
				{
					addCommandKeys(cmdIndex, keyConfig.keyBindings[i]);
				}
				else
				{
					setCommandKeys(cmdIndex, i, keyConfig.keyBindings[i]);
				}
			}
		}
	}

	void CommandManager::updateWindow()
	{
		if (ImGui::Begin("Input Bindings"))
		{
			ImVec2 size = ImVec2(-1, ImGui::GetContentRegionAvail().y - 200);
			ImGui::BeginChild("##command_select_window", size, true);
			for (int i = 0; i < commands.size(); ++i)
			{
				std::string itemText = commands[i].getDisplayName() + "(" + commands[i].getAllKeysString() + ")";
				if (ImGui::Selectable(itemText.c_str(), selectedCommandIndex == i, ImGuiSelectableFlags_SpanAvailWidth))
				{
					selectedCommandIndex = i;
				}

				ImGui::Separator();
			}
			ImGui::EndChild();
		}

		if (selectedCommandIndex > -1)
		{
			int deleteBinding = -1;
			if (ImGui::Button("Add", ImVec2(-1, UI::btnSmall.y)))
				commands[selectedCommandIndex].addKeys(CommandKeys{});

			ImGui::BeginChild("##command_edit_windw", ImVec2(-1, -1), true);
			for (int b = 0; b < commands[selectedCommandIndex].getBindingsCount(); ++b)
			{
				ImGui::PushID(b);

				std::string buttonText;
				std::string cmdText = commands[selectedCommandIndex].getKeysString(b);
				if (cmdText == "")
					cmdText = "None";

				buttonText = listeningForInput && editBindingIndex == b ? "Listening for input..." : cmdText;
				if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - 5, UI::btnSmall.y)))
				{
					listeningForInput = true;
					inputTimer.reset();
					editBindingIndex = b;
				}

				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_TRASH, UI::btnSmall))
					deleteBinding = b;

				ImGui::PopID();
			}
			ImGui::EndChild();

			if (deleteBinding > -1)
			{
				listeningForInput = false;
				commands[selectedCommandIndex].removeKeys(deleteBinding);
			}
		}

		ImGui::End();

		if (listeningForInput)
		{
			if (inputTimer.elapsed() >= inputTimeoutSeconds)
			{
				listeningForInput = false;
				editBindingIndex = -1;
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

					commands[selectedCommandIndex].setKeys(editBindingIndex, CommandKeys{ mods, i });
					listeningForInput = false;
					editBindingIndex = -1;
				}
			}
		}
	}
}