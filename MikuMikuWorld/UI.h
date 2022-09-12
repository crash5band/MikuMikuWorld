#pragma once
#include "ImGui/imgui.h"
#include "ImGui/imgui_stdlib.h"
#include "ImGui/imgui_internal.h"
#include "Rendering/Texture.h"
#include "IconsFontAwesome5.h"
#include "Localization.h"
#include <vector>

#define APP_NAME "MikuMikuWorld"
#define APP_NAME_FRIENDLY "Miku Miku World"

namespace MikuMikuWorld
{
	constexpr const char* toolboxWindowTitle = ICON_FA_TOOLBOX " Toolbox";
	constexpr const char* controlsWindowTitle = ICON_FA_ADJUST " Controls";
	constexpr const char* timelineWindowTitle = ICON_FA_MUSIC " Notes Timeline";
	constexpr const char* detailsWindowTitle = ICON_FA_ALIGN_LEFT " Chart Properties";
	constexpr const char* presetsWindowTitle = ICON_FA_DRAFTING_COMPASS " Presets";
	constexpr const char* Title = ICON_FA_BUG " Debug";
	constexpr const char* windowTitle = " - " APP_NAME;
	constexpr const char* windowUntitled = "Untitled";
	constexpr const char* aboutModalTitle = "About - " APP_NAME_FRIENDLY;
	constexpr const char* settingsModalTitle = "Settings - " APP_NAME_FRIENDLY;
	constexpr const char* unsavedModalTitle = "Unsaved Changes - " APP_NAME_FRIENDLY;

	constexpr float primaryLineThickness = 0.50f;
	constexpr float secondaryLineThickness = 0.25f;
	constexpr float toolTipDelay = 0.5f;

	constexpr ImGuiWindowFlags ImGuiWindowFlags_Static = ImGuiWindowFlags_NoCollapse;

	struct AccentColor
	{
		std::string name;
		ImVec4 color;
	};

	enum class ColorDisplay
	{
		RGB,
		HSV,
		HEX
	};

	constexpr const char* colorDisplayStr[]
	{
		"RGB", "HSV", "Hex"
	};

	class UI
	{
	public:
		static const ImVec2 btnNormal;
		static const ImVec2 btnSmall;
		static std::vector<AccentColor> accentColors;

		static bool transparentButton(const char* txt, ImVec2 size = btnNormal, bool repeat = false, bool enabled = true);
		static bool transparentButton2(const char* txt, ImVec2 pos, ImVec2 size);
		static bool isAnyPopupOpen();
		static void beginPropertyColumns();
		static void endPropertyColumns();
		static void propertyLabel(const char* label);
		static void addStringProperty(const char* label, std::string& val);
		static void addIntProperty(const char* label, int& val, int lowerBound = 0, int higherBound = 0);
		static void addFloatProperty(const char* label, float& val, const char* format);
		static void addReadOnlyProperty(const char* label, std::string val);
		static void addSliderProperty(const char* label, int& val, int min, int max, const char* format);
		static void addSliderProperty(const char* label, float& val, float min, float max, const char* format);
		static void addPercentSliderProperty(const char* label, float& val);
		static bool addFractionProperty(const char* label, int& numerator, int& denominator);
		static int addFileProperty(const char* label, std::string& val);
		static void addMultilineString(const char* label, std::string& val);
		static void tooltip(const char* label);
		static void textureTooltip(const Texture& tex);

		static void setWindowTitle(const std::string& title);

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