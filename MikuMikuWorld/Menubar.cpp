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
		commandsCache["reset"] = commandManager->findCommand("reset");
		commandsCache["open"] = commandManager->findCommand("open");
		commandsCache["save"] = commandManager->findCommand("save");
		commandsCache["save_as"] = commandManager->findCommand("save_as");
		commandsCache["export"] = commandManager->findCommand("export");

		commandsCache["undo"] = commandManager->findCommand("undo");
		commandsCache["redo"] = commandManager->findCommand("redo");
		commandsCache["select_all"] = commandManager->findCommand("select_all");
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
			Command& reset = commandManager->commands[commandsCache["reset"]];
			Command& open = commandManager->commands[commandsCache["open"]];
			Command& save = commandManager->commands[commandsCache["save"]];
			Command& saveAs = commandManager->commands[commandsCache["save_as"]];
			Command& exportSUS = commandManager->commands[commandsCache["export"]];

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
			Command& undo = commandManager->commands[commandsCache["undo"]];
			Command& redo = commandManager->commands[commandsCache["redo"]];
			Command& selectAll = commandManager->commands[commandsCache["select_all"]];

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