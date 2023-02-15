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
			{
				// Do not add empty key bindings
				if (command.getKey(i) != 0)
					bindings.push_back(command.getKeysString(i));
			}

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

	void CommandManager::inputSettings()
	{
		ImVec2 size = ImVec2(-1, ImGui::GetContentRegionAvail().y * 0.7);
		const ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;
		const ImGuiSelectableFlags selectionFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
		int rowHeight = ImGui::GetFrameHeight() + 2;
		
		if (ImGui::BeginTable("##commands_table", 2, tableFlags, size))
		{
			ImGui::TableSetupColumn("Command");
			ImGui::TableSetupColumn("Bindings");
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableHeadersRow();

			for (int c = 0; c < commands.size(); ++c)
			{
				ImGui::TableNextRow(rowHeight);
				
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Selectable(commands[c].getDisplayName().c_str(), c == selectedCommandIndex, selectionFlags))
					selectedCommandIndex = c;

				ImGui::TableSetColumnIndex(1);
				ImGui::Text(commands[c].getAllKeysString().c_str());
			}
			ImGui::EndTable();
		}
		ImGui::Separator();

		if (selectedCommandIndex > -1)
		{
			int deleteBinding = -1;
			ImGui::Text("Bound Keys");

			UI::beginPropertyColumns();
			ImGui::Text("Command: %s", commands[selectedCommandIndex].getDisplayName().c_str());
			ImGui::NextColumn();
			if (ImGui::Button("Add", ImVec2(-1, UI::btnSmall.y)))
				commands[selectedCommandIndex].addKeys(CommandKeys{});

			UI::endPropertyColumns();

			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
			ImGui::BeginChild("##command_edit_window", ImVec2(-1, -1), true);
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
			ImGui::PopStyleColor();

			if (deleteBinding > -1)
			{
				listeningForInput = false;
				commands[selectedCommandIndex].removeKeys(deleteBinding);
			}
		}
		else
		{
			ImGui::Text("Select a command to edit it's key bindings.");
		}

		if (listeningForInput)
		{
			if (inputTimer.elapsed() >= inputTimeoutSeconds)
			{
				listeningForInput = false;
				editBindingIndex = -1;
			}

			for (int key = GLFW_KEY_SPACE; key < GLFW_KEY_LAST; ++key)
			{
				const bool isControl = key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL;
				const bool isShift = key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT;
				const bool isAlt = key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT;

				// execute if a non-modifier key is tapped
				if (!isControl && !isShift && !isAlt && InputListener::isTapped(key))
				{
					int mods = 0;
					if (InputListener::isCtrlDown()) mods |= CTRL;
					if (InputListener::isShiftDown()) mods |= SHIFT;
					if (InputListener::isAltDown()) mods |= ALT;

					commands[selectedCommandIndex].setKeys(editBindingIndex, CommandKeys{ mods, key });
					listeningForInput = false;
					editBindingIndex = -1;
				}
			}
		}
	}
}