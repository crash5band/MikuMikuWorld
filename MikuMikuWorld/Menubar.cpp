#include "Menubar.h"
#include "ImGui/imgui.h"
#include "Localization.h"
#include <GLFW/glfw3.h>

namespace MikuMikuWorld
{
	Menubar::Menubar()
	{

	}

	void Menubar::setCommandManagerInstance(CommandManager* commandManager)
	{
		this->commandManager = commandManager;
		commandsCache["cmd_reset"] = commandManager->findCommand("cmd_reset");
		commandsCache["cmd_open"] = commandManager->findCommand("cmd_open");
		commandsCache["cmd_save"] = commandManager->findCommand("cmd_save");
		commandsCache["cmd_save_as"] = commandManager->findCommand("cmd_save_as");
		commandsCache["cmd_export"] = commandManager->findCommand("cmd_export");

		commandsCache["cmd_undo"] = commandManager->findCommand("cmd_undo");
		commandsCache["cmd_redo"] = commandManager->findCommand("cmd_redo");
		commandsCache["cmd_select_all"] = commandManager->findCommand("cmd_select_all");
	}

	void Menubar::menuItem(std::string label, Command& cmd, bool selected)
	{
		if (ImGui::MenuItem(getString(label), cmd.getKeysString(0).c_str(), selected, cmd.canExecute()))
			cmd.execute();
	}

	void Menubar::update(bool& settingsOpen, bool& aboutOpen, bool& vsync, bool& exiting, bool& showPerformanceMetrics)
	{
		ImGui::BeginMainMenuBar();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 3));

		if (ImGui::BeginMenu(getString("file")))
		{
			Command& reset = commandManager->commands[commandsCache["cmd_reset"]];
			Command& open = commandManager->commands[commandsCache["cmd_open"]];
			Command& save = commandManager->commands[commandsCache["cmd_save"]];
			Command& saveAs = commandManager->commands[commandsCache["cmd_save_as"]];
			Command& exportSUS = commandManager->commands[commandsCache["cmd_export"]];

			menuItem("new", reset);
			menuItem("open", open);
			ImGui::Separator();

			menuItem("save", save);
			menuItem("save_as", saveAs);
			menuItem("export", exportSUS);
			ImGui::Separator();

			if (ImGui::MenuItem(getString("exit"), "Alt + F4"))
				exiting = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("edit")))
		{
			Command& undo = commandManager->commands[commandsCache["cmd_undo"]];
			Command& redo = commandManager->commands[commandsCache["cmd_redo"]];
			Command& selectAll = commandManager->commands[commandsCache["cmd_select_all"]];

			menuItem("undo", undo);
			menuItem("redo", redo);
			ImGui::Separator();

			menuItem("select_all", selectAll);

			if (ImGui::MenuItem(getString("settings")))
				settingsOpen = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("window")))
		{
			if (ImGui::MenuItem(getString("vsync"), NULL, &vsync))
				glfwSwapInterval((int)vsync);

			ImGui::MenuItem(getString("show_performance"), NULL, &showPerformanceMetrics);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("about")))
		{
			if (ImGui::MenuItem(getString("about")))
				aboutOpen = true;

			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}
}