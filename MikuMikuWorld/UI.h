#pragma once
#include "IO.h"
#include "IconsFontAwesome5.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_stdlib.h"
#include "Localization.h"
#include "NoteTypes.h"
#include <vector>

#define APP_NAME "MikuMikuWorld for Chart Cyanvas"

#define IMGUI_TITLE(icon, title)                                                                   \
	IO::formatString("%s %s###%s", icon, getString(title), title).c_str()
#define MODAL_TITLE(title)                                                                         \
	IO::formatString("%s - %s###%s", APP_NAME, getString(title), title).c_str()

namespace MikuMikuWorld
{
	constexpr const char* windowUntitled = "Untitled";

	constexpr float primaryLineThickness = 0.250f;
	constexpr float secondaryLineThickness = 0.150f;
	constexpr float tertiaryLineThickness = 0.100f;
	constexpr float toolTipDelay = 0.5f;

	constexpr ImGuiWindowFlags ImGuiWindowFlags_Static = ImGuiWindowFlags_NoCollapse;
	constexpr ImGuiWindowFlags ImGuiWindowFlags_Toolbar =
	    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
	    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar;

	enum class ColorDisplay : uint8_t
	{
		RGB,
		HSV,
		HEX
	};

	enum class BaseTheme : uint8_t
	{
		DARK,
		LIGHT,
		BASE_THEME_MAX
	};

	constexpr const char* colorDisplayStr[]{ "RGB", "HSV", "Hex" };

	constexpr const char* baseThemes[]{ "theme_dark", "theme_light" };

	class UI
	{
	  private:
		static char idStr[256];
		static const ImVec2 _btnNormal;
		static const ImVec2 _btnSmall;
		static const ImVec2 _toolbarBtnSize;
		static const ImVec2 _toolbarBtnImgSize;

	  public:
		static ImVec2 btnNormal;
		static ImVec2 btnSmall;
		static ImVec2 toolbarBtnSize;
		static ImVec2 toolbarBtnImgSize;
		static std::vector<ImVec4> accentColors;

		static bool transparentButton(const char* txt, ImVec2 size = btnNormal, bool repeat = false,
		                              bool enabled = true);
		static bool transparentButton2(const char* txt, ImVec2 pos, ImVec2 size);
		static bool coloredButton(const char* txt, ImVec2 pos, ImVec2 size, ImU32 col,
		                          bool enabled = true);
		static bool isAnyPopupOpen();
		static void beginPropertyColumns();
		static void endPropertyColumns();
		static void propertyLabel(const char* label);
		static void addIntProperty(const char* label, int& val, int lowerBound = 0,
		                           int higherBound = 0);
		static void addIntProperty(const char* label, int& val, const char* format,
		                           int lowerBound = 0, int higherBound = 0);
		static void addFloatProperty(const char* label, float& val, const char* format);
		static void addStringProperty(const char* label, std::string& val);
		static void addSliderProperty(const char* label, int& val, int min, int max,
		                              const char* format);
		static void addSliderProperty(const char* label, float& val, float min, float max,
		                              const char* format);
		static void addDragFloatProperty(const char* label, float& val, const char* format);
		static void addPercentSliderProperty(const char* label, float& val);
		static bool addFractionProperty(const char* label, int& numerator, int& denominator);
		static void addCheckboxProperty(const char* label, bool& val);
		static int addFileProperty(const char* label, std::string& val);
		static void addMultilineString(const char* label, std::string& val);
		static void tooltip(const char* label);
		static const char* labelID(const char* label);
		static bool divisionSelect(const char* label, int& value, const int* items, size_t count);
		static bool zoomControl(const char* label, float& value, float min, float max, float width);
		static bool timeSignatureSelect(int& numerator, int& denominator);
		static bool toolbarButton(const char* icon, const char* label, const char* shortcut,
		                          bool enabled = true, bool selected = false);
		static bool toolbarImageButton(const char* img, const char* label, const char* shortcut,
		                               bool enabled = true, bool selected = false);
		static void toolbarSeparator();
		static void beginNextItemDisabled();
		static void endNextItemDisabled();

		static void setWindowTitle(std::string title);
		static void updateBtnSizesDpiScaling(float scale);

		template <typename T> static void addReadOnlyProperty(const char* label, T val)
		{
			propertyLabel(label);
			ImGui::Text(std::to_string(val).c_str());
			ImGui::NextColumn();
		}

		static void addReadOnlyProperty(const char* label, const char* val)
		{
			propertyLabel(label);
			ImGui::Text(val);
			ImGui::NextColumn();
		}

		inline static void addReadOnlyProperty(const char* label, const std::string& val)
		{
			addReadOnlyProperty(label, val.c_str());
		}

		/// <summary>
		/// For use with sequential enums only
		/// </summary>
		template <typename T>
		static void addSelectProperty(const char* label, T& value, const char* const* items,
		                              int count)
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

		// we need to get proper translated names for the colors
		template <>
		static void addSelectProperty<GuideColor>(const char* label, GuideColor& value,
		                                         const char* const* items, int count)
		{
			propertyLabel(label);

			std::string id("##");
			id.append(label);

			std::string curr = items[(int)value];
			if (!curr.size())
				curr = items[(int)value];
			const std::string translated_str = getString(curr.c_str());
			if (ImGui::BeginCombo(id.c_str(), translated_str.c_str()))
			{
				for (int i = (int)GuideColor::Neutral; i < count; ++i)
				{
					const bool selected = (int)value == i;
					std::string str = getString(items[i]);
					if (!str.size())
						str = items[i];

					if (ImGui::Selectable(str.c_str(), selected))
						value = (GuideColor)i;
				}

				ImGui::EndCombo();
			}

			ImGui::NextColumn();
		}

		// we don't want FlickType::None to appear in the selection
		template <>
		static void addSelectProperty<FlickType>(const char* label, FlickType& value,
		                                         const char* const* items, int count)
		{
			propertyLabel(label);

			std::string id("##");
			id.append(label);

			std::string curr = getString(items[(int)value]);
			if (!curr.size())
				curr = items[(int)value];
			if (ImGui::BeginCombo(id.c_str(), curr.c_str()))
			{
				for (int i = (int)FlickType::Default; i < count; ++i)
				{
					const bool selected = (int)value == i;
					std::string str = getString(items[i]);
					if (!str.size())
						str = items[i];

					if (ImGui::Selectable(str.c_str(), selected))
						value = (FlickType)i;
				}

				ImGui::EndCombo();
			}

			ImGui::NextColumn();
		}

		static void addFlickSelectPropertyWithNone(const char* label, FlickType& value,
		                                         const char* const* items, int count)
		{
			propertyLabel(label);

			std::string id("##");
			id.append(label);

			std::string curr = getString(items[(int)value]);
			if (!curr.size())
				curr = items[(int)value];
			if (ImGui::BeginCombo(id.c_str(), curr.c_str()))
			{
				for (int i = (int)FlickType::None; i < count; ++i)
				{
					const bool selected = (int)value == i;
					std::string str = getString(items[i]);
					if (!str.size())
						str = items[i];

					if (ImGui::Selectable(str.c_str(), selected))
						value = (FlickType)i;
				}

				ImGui::EndCombo();
			}

			ImGui::NextColumn();
		}
	};
}
