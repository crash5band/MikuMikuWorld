#include "Toolbar.h"
#include "ImGui/imgui.h"
#include "UI.h"
#include "ResourceManager.h"
#include "Commands/CommandManager.h"
#include "ImGui/imgui_internal.h"

namespace MikuMikuWorld
{
	void toolbarButton(CommandManager& cmdManager, std::string cmdName, const char* label)
	{
		int cmdIndex = cmdManager.findCommand(cmdName);
		if (cmdIndex == -1)
			return;

		bool canExecute = cmdManager.commands[cmdIndex].canExecute();
		if (!canExecute)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * true));
		}

		if (ImGui::Button(label, UI::toolbarBtnSize))
			cmdManager.commands[cmdIndex].execute();

		if (!canExecute)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		UI::tooltip(cmdManager.commands[cmdIndex].getDisplayName().c_str());
		ImGui::SameLine();
	}

	void toolbarImageButton(CommandManager& cmdManager, std::string cmdName, std::string texName, bool selected)
	{
		int cmdIndex = cmdManager.findCommand(cmdName);
		if (cmdIndex == -1)
			return;

		std::string btnLabelId = "##" + cmdName;
		int texId = 0;
		int texIndex = ResourceManager::getTexture(texName);
		if (texIndex != -1)
			texId = ResourceManager::textures[texIndex].getID();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0, 0 });
		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
		}

		if (ImGui::ImageButton(btnLabelId.c_str(), (void*)texId, UI::toolbarBtnSize))
			cmdManager.commands[cmdIndex].execute();

		UI::tooltip(cmdManager.commands[cmdIndex].getDisplayName().c_str());

		if (selected)
			ImGui::PopStyleColor(2);

		ImGui::PopStyleVar();
		ImGui::SameLine();
	}

	void toolbarSeparator()
	{
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
	}
}
