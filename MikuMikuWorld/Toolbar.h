#pragma once
#include <string>

namespace MikuMikuWorld
{
	class CommandManager;

	void toolbarButton(CommandManager& cmdManager, std::string cmdName, const char* label);
	void toolbarImageButton(CommandManager& cmdManager, std::string cmdName, std::string texName, bool selected);
	void toolbarSeparator();
}