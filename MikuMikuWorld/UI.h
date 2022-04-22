#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/imgui_internal.h"
#include "IconsFontAwesome5.h"
#include <vector>

#define APP_NAME "MikuMikuWorld"
#define APP_NAME_FRIENDLY "Miku Miku World"

namespace MikuMikuWorld
{
	constexpr const char* toolboxWindow = ICON_FA_TOOLBOX " Toolbox";
	constexpr const char* controlsWindow = ICON_FA_ADJUST " Controls";
	constexpr const char* timelineWindow = ICON_FA_MUSIC " Notes Timeline";
	constexpr const char* detailsWindow = ICON_FA_ALIGN_LEFT " Chart Properties";
	constexpr const char* debugWindow = ICON_FA_BUG " Debug";
	constexpr const char* windowTitle = APP_NAME " - ";
	constexpr const char* windowTitleNew = APP_NAME " - Untitled";
	constexpr const char* aboutModalTitle = APP_NAME_FRIENDLY " - About";
	constexpr const char* settingsModalTitle = APP_NAME_FRIENDLY " - Settings";
	constexpr const char* unsavedModalTitle = APP_NAME_FRIENDLY " - Unsaved Changes";

	constexpr ImGuiWindowFlags ImGuiWindowFlags_Static = ImGuiWindowFlags_NoCollapse;

	static const ImVec2 btnNormal{ 30, 30 };
	static const ImVec2 btnSmall{ 20, 20 };

	struct AccentColor
	{
		std::string name;
		ImVec4 color;
	};

	extern std::vector<AccentColor> accentColors;

	bool transparentButton(const char* txt, ImVec2 size = btnNormal, bool repeat = false, bool enabled = true);
	bool transparentButton2(const char* txt, ImVec2 pos, ImVec2 size);
	bool isAnyPopupOpen();
	void beginPropertyColumns();
	void endPropertyColumns();
	void propertyLabel(const char* label);
	void addStringProperty(const char* label, std::string& val);
	void addIntProperty(const char* label, int& val, int lowerBound = 0, int higherBound = 0);
	void addFloatProperty(const char* label, float& val, const char* format);
	void addReadOnlyProperty(const char* label, std::string val);
	void addSliderProperty(const char* label, int& val, int min, int max, const char* format);
	void addSliderProperty(const char* label, float& val, float min, float max, const char* format);
	void addPercentSliderProperty(const char* label, float &val);
	bool addFractionProperty(const char* label, int& numerator, int& denominator);
	bool addFileProperty(const char* label, std::string val);

	void setWindowTitle(const std::string& title);
}