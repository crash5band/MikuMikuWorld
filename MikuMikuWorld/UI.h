#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/imgui_internal.h"
#include "IconsFontAwesome5.h"
#include "Localization.h"
#include "IO.h"
#include <vector>

#define APP_NAME "MikuMikuWorld"

#define IMGUI_TITLE(icon, title) IO::concat(icon, getString(title), " ").c_str()
#define MODAL_TITLE(title) IO::concat(APP_NAME, getString(title), " - ").c_str()

namespace MikuMikuWorld
{
	constexpr const char* windowTitle = APP_NAME " - ";
	constexpr const char* windowUntitled = "Untitled";

	constexpr float primaryLineThickness = 1.00f;
	constexpr float secondaryLineThickness = 0.75f;
	constexpr float toolTipDelay = 0.5f;

	constexpr ImGuiWindowFlags ImGuiWindowFlags_Static = ImGuiWindowFlags_NoCollapse;
	constexpr ImGuiWindowFlags ImGuiWindowFlags_Toolbar = ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoTitleBar;

	enum class ColorDisplay
	{
		RGB,
		HSV,
		HEX
	};

	enum class BaseTheme
	{
		DARK,
		LIGHT,
		BASE_THEME_MAX
	};

	constexpr const char* colorDisplayStr[]
	{
		"RGB", "HSV", "Hex"
	};

	constexpr const char* baseThemes[]
	{
		"theme_dark", "theme_light"
	};

	class UI
	{
	private:
		static char idStr[256];

	public:
		static const ImVec2 btnNormal;
		static const ImVec2 btnSmall;
		static const ImVec2 toolbarBtnSize;
		static std::vector<ImVec4> accentColors;

		static bool transparentButton(const char* txt, ImVec2 size = btnNormal, bool repeat = false, bool enabled = true);
		static bool transparentButton2(const char* txt, ImVec2 pos, ImVec2 size);
		static bool coloredButton(const char* txt, ImVec2 pos, ImVec2 size, ImU32 col, bool enabled = true);
		static bool isAnyPopupOpen();
		static void beginPropertyColumns();
		static void endPropertyColumns();
		static void propertyLabel(const char* label);
		static void addIntProperty(const char* label, int& val, int lowerBound = 0, int higherBound = 0);
		static void addFloatProperty(const char* label, float& val, const char* format);
		static void addStringProperty(const char* label, std::string& val);
		static void addSliderProperty(const char* label, int& val, int min, int max, const char* format);
		static void addSliderProperty(const char* label, float& val, float min, float max, const char* format);
		static void addPercentSliderProperty(const char* label, float& val);
		static bool addFractionProperty(const char* label, int& numerator, int& denominator);
		static void addCheckboxProperty(const char* label, bool& val);
		static int addFileProperty(const char* label, std::string& val);
		static void addMultilineString(const char* label, std::string& val);
		static void tooltip(const char* label);
		static const char* labelID(const char* label);
		static bool divisionSelect(const char* label, int& value, const int* items, size_t count);
		static bool zoomControl(const char* label, float& value, float min, float max);
		static bool timeSignatureSelect(int& numerator, int& denominator);
		static bool toolbarButton(const char* icon, const char* label, const char* shortcut, bool enabled = true, bool selected = false);
		static bool toolbarImageButton(const char* img, const char* label, const char* shortcut, bool enabled = true, bool selected = false);
		static void toolbarSeparator();
		static void beginNextItemDisabled();
		static void endNextItemDisabled();

		static void setWindowTitle(std::string title);

		template <typename T>
		static void addReadOnlyProperty(const char* label, T val)
		{
			propertyLabel(label);
			ImGui::Text(std::to_string(val).c_str());
			ImGui::NextColumn();
		}

		/// <summary>
		/// For use with sequential enums only
		/// </summary>
		template <typename T>
		static void addSelectProperty(const char* label, T& value, const char* const* items, int count)
		{
			propertyLabel(label);

			std::string id("##");
			id.append(label);

			std::string curr = getString(items[(int)value]);
			if (!curr.size())
				curr = items[(int)value];
			if (ImGui::BeginCombo(id.c_str(), curr.c_str()))
			{
				for (int i = 0; i < count; ++i)
				{
					const bool selected = (int)value == i;
					std::string str = getString(items[i]);
					if (!str.size())
						str = items[i];

					if (ImGui::Selectable(str.c_str(), selected))
						value = (T)i;
				}

				ImGui::EndCombo();
			}

			ImGui::NextColumn();
		}
	};
}