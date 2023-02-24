#include "Menubar.h"
#include "ImGui/imgui.h"
#include "Localization.h"
#include <GLFW/glfw3.h>

#ifdef _MSC_VER
#include <windows.h>
#include <shellapi.h>
#endif // _MSC_VER


namespace MikuMikuWorld
{
	Menubar::Menubar()
		: commandManager{ nullptr }
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

	void Menubar::menuCommand(Command& cmd, bool selected)
	{
		if (ImGui::MenuItem(cmd.getDisplayName().c_str(), cmd.getKeysString(0).c_str(), selected, cmd.canExecute()))
			cmd.execute();
	}

	void Menubar::openHelpPage()
	{
		ShellExecuteW(0, 0, L"https://github.com/crash5band/MikuMikuWorld/wiki", 0, 0, SW_SHOW);
	}

	void Menubar::update(WindowState& state)
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

			menuCommand(reset);
			menuCommand(open);
			ImGui::Separator();

			menuCommand(save);
			menuCommand(saveAs);
			menuCommand(exportSUS);
			ImGui::Separator();

			if (ImGui::MenuItem(getString("exit"), "Alt + F4"))
				state.closing = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("edit")))
		{
			Command& undo = commandManager->commands[commandsCache["undo"]];
			Command& redo = commandManager->commands[commandsCache["redo"]];
			Command& selectAll = commandManager->commands[commandsCache["select_all"]];

			menuCommand(undo);
			menuCommand(redo);
			ImGui::Separator();

			menuCommand(selectAll);

			if (ImGui::MenuItem(getString("settings")))
				state.settingsOpen = true;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("window")))
		{
			if (ImGui::MenuItem(getString("vsync"), NULL, &state.vsync))
				glfwSwapInterval((int)state.vsync);

			ImGui::MenuItem(getString("show_performance"), NULL, &state.showPerformanceMetrics);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu(getString("help")))
		{
			if (ImGui::MenuItem(getString("help")))
				openHelpPage();

			if (ImGui::MenuItem(getString("about")))
				state.aboutOpen = true;

			ImGui::EndMenu();
		}

		ImGui::PopStyleVar();
		ImGui::EndMainMenuBar();
	}
}