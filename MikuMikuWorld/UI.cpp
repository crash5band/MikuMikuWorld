#include "UI.h"
#include "Utilities.h"
#include <string>
#include <algorithm>
#include <GLFW/glfw3.h>

namespace MikuMikuWorld
{
	const ImVec2 UI::btnNormal{ 30, 30 };
	const ImVec2 UI::btnSmall{ 22, 22 };
	char UI::idStr[256];

	std::vector<AccentColor> UI::accentColors{
		AccentColor{ "User",			ImVec4(0.10f, 0.10f, 0.10f, 1.00f) },
		AccentColor{ "Default",			ImVec4(0.15f, 0.46f, 0.82f, 1.00f) },
		AccentColor{ "MikuMikuWorld",	ImVec4(0.19f, 0.75f, 0.62f, 1.00f) },
		AccentColor{ "Light Music",		ImVec4(0.30f, 0.31f, 0.86f, 1.00f) },
		AccentColor{ "Idol", 			ImVec4(0.40f, 0.69f, 0.15f, 1.00f) },
		AccentColor{ "Street", 			ImVec4(0.76f, 0.05f, 0.32f, 1.00f) },
		AccentColor{ "Theme Park", 		ImVec4(0.81f, 0.45f, 0.06f, 1.00f) },
		AccentColor{ "School Refusal",	ImVec4(0.50f, 0.25f, 0.55f, 1.00f) },
		AccentColor{ "Plain",			ImVec4(0.40f, 0.40f, 0.40f, 1.00f) }
	};

	int filterNumsOnly(ImGuiInputTextCallbackData* data)
	{
		if (!std::isdigit(data->EventChar))
			return 1;

		return 0;
	}

	const char* UI::labelID(const char* label)
	{
		strcpy(idStr, "##");
		strcpy(idStr + 2, label);
		return idStr;
	}

	bool UI::transparentButton(const char* txt, ImVec2 size, bool repeat, bool enabled)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !enabled);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * !enabled));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

		ImGui::PushButtonRepeat(repeat);
		bool pressed = ImGui::Button(txt, size);
		ImGui::PopButtonRepeat();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();

		return pressed;
	}

	bool UI::transparentButton2(const char* txt, ImVec2 pos, ImVec2 size)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.4f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

		ImGui::SetCursorScreenPos(pos);
		bool pressed = ImGui::Button(labelID(txt), size);

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		return pressed;
	}

	bool UI::isAnyPopupOpen()
	{
		return ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
	}

	void UI::beginPropertyColumns()
	{
		ImGui::Columns(2);
	}

	void UI::endPropertyColumns()
	{
		ImGui::Columns(1);
	}

	void UI::propertyLabel(const char* label)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::SetNextItemWidth(-1);
	}

	void UI::addStringProperty(const char* label, std::string& val)
	{
		propertyLabel(label);

		ImGui::InputText(labelID(label), &val);
		ImGui::NextColumn();
	}

	void UI::addIntProperty(const char* label, int& val, int lowerBound, int higherBound)
	{
		propertyLabel(label);
		
		ImGui::InputInt(labelID(label), &val, 1, 5);
		if (lowerBound != higherBound)
			val = std::clamp(val, lowerBound, higherBound);
		ImGui::NextColumn();
	}

	void UI::addFloatProperty(const char* label, float& val, const char* format)
	{
		propertyLabel(label);
		
		ImGui::InputFloat(labelID(label), &val, 1.0f, 10.f, format);
		ImGui::NextColumn();
	}

	void UI::addReadOnlyProperty(const char* label, std::string val)
	{
		propertyLabel(label);
		ImGui::Text(val.c_str());
		ImGui::NextColumn();
	}

	void UI::addSliderProperty(const char* label, int& val, int min, int max, const char* format)
	{
		propertyLabel(label);

		ImGui::SliderInt(labelID(label), &val, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();
	}

	void UI::addSliderProperty(const char* label, float& val, float min, float max, const char* format)
	{
		propertyLabel(label);

		ImGui::SliderFloat(labelID(label), &val, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();
	}

	void UI::addPercentSliderProperty(const char* label, float &val)
	{
		propertyLabel(label);

		float scaled = val * 100;
		ImGui::SliderFloat(labelID(label), &scaled, 0, 100, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();

		val = scaled / 100.0f;
	}

	int UI::addFileProperty(const char* label, std::string& val)
	{
		propertyLabel(label);
		
		int result = 0;

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::InputTextWithHint(labelID(label), "n/a", &val, ImGuiInputTextFlags_EnterReturnsTrue))
			result = 1;
		ImGui::SameLine();

		ImGui::PushID(label);
		if (ImGui::Button("...", btnSmall))
			result = 2;

		ImGui::PopID();
		ImGui::NextColumn();

		return result;
	}

	bool UI::addFractionProperty(const char* label, int& numerator, int& denominator)
	{
		propertyLabel(label);

		const char* id = labelID(label);
		std::string idNumerator{ id };
		std::string idDenominator{ id };
		idNumerator.append("_numerator");
		idDenominator.append("_denominator");

		float controlWidth = (ImGui::GetContentRegionAvail().x / 2.0f) - (ImGui::CalcTextSize("/").x);
		bool edit = false;
		
		ImGui::SetNextItemWidth(controlWidth);
		ImGui::InputInt(idNumerator.c_str(), &numerator, 0, 0);

		edit |= ImGui::IsItemDeactivatedAfterEdit();

		ImGui::SameLine();
		ImGui::Text("/");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(controlWidth);
		ImGui::InputInt(idDenominator.c_str(), &denominator, 0, 0);

		edit |= ImGui::IsItemDeactivatedAfterEdit();
		ImGui::NextColumn();

		return edit;
	}

	void UI::addMultilineString(const char* label, std::string& val)
	{
		propertyLabel(label);

		ImGui::InputTextMultiline(labelID(label), &val, ImVec2(-1, 50));
		ImGui::NextColumn();
	}

	void UI::tooltip(const char* label)
	{
		if (ImGui::IsItemHovered() && GImGui->HoveredIdTimer > 0.5f)
		{
			float txtWidth = ImGui::CalcTextSize(label).x + (ImGui::GetStyle().WindowPadding.x * 2);
			ImGui::SetNextWindowSize(ImVec2(std::min(txtWidth, 250.0f), -1));
			ImGui::BeginTooltipEx(ImGuiTooltipFlags_OverridePreviousTooltip, ImGuiWindowFlags_NoResize);
			ImGui::TextWrapped(label);
			ImGui::EndTooltip();
		}
	}

	bool UI::divisionSelect(const char* label, int& value, const int* items, size_t count)
	{
		propertyLabel(label);
		
		const char* id = labelID(label);
		ImGui::PushID(id);

		bool act = false;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight());
		if (ImGui::InputScalar(id, ImGuiDataType_U32, (void*)(&value), 0, 0, Utilities::getDivisionString(value).c_str()))
		{
			value = std::clamp(value, 4, 1920);
			act = true;
		}
		
		// enable right-click
		ImGui::OpenPopupOnItemClick("combobox");
		ImVec2 pos = ImGui::GetItemRectMin();
		ImVec2 size = ImGui::GetItemRectSize();

		ImGui::SameLine(0, 0);
		if (ImGui::ArrowButton("##open_combo", ImGuiDir_Down))
			ImGui::OpenPopup("combobox");

		// enable right-click
		ImGui::OpenPopupOnItemClick("combobox");
		
		pos.y += size.y;
		size.x += ImGui::GetItemRectSize().x;
		size.y += size.y * 6;
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(size);
		if (ImGui::BeginPopup("combobox", ImGuiWindowFlags_NoMove))
		{
			for (int i = 0; i < count; ++i)
			{
				if (ImGui::Selectable(Utilities::getDivisionString(items[i]).c_str(), value == items[i]))
				{
					value = items[i];
					act = true;
				}
			}

			ImGui::EndPopup();
		}

		ImGui::PopID();
		ImGui::NextColumn();
		return act;
	}

	bool UI::zoomControl(const char* label, float& value, float min, float max)
	{
		propertyLabel(getString("zoom"));
		
		bool act = false;
		if (UI::transparentButton(ICON_FA_SEARCH_MINUS, UI::btnSmall))
		{
			value -= 0.25f;
			act = true;
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - (ImGui::GetStyle().ItemSpacing.x));

		act |= ImGui::SliderFloat(labelID(label), &value, min, max, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
		ImGui::SameLine();

		if (UI::transparentButton(ICON_FA_SEARCH_PLUS, UI::btnSmall))
		{
			value += 0.25f;
			act = true;
		}

		value = std::clamp(value, min, max);
		return act;
	}

	void UI::setWindowTitle(std::string title)
	{
		GLFWwindow* window = glfwGetCurrentContext();
		if (!window)
			return;

		glfwSetWindowTitle(window, title.append(windowTitle).c_str());
	}
}
