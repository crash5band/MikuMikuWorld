#include "UI.h"
#include "Utilities.h"
#include "Colors.h"
#include "Tempo.h"
#include "ResourceManager.h"
#include "TimelineMode.h"
#include <string>
#include <algorithm>
#include <GLFW/glfw3.h>

namespace MikuMikuWorld
{
	const ImVec2 UI::_btnNormal{ 24, 24 };
	const ImVec2 UI::_btnSmall{ 18, 18 };
	const ImVec2 UI::_toolbarBtnSize{ 22, 22 };
	const ImVec2 UI::_toolbarBtnImgSize{ 19, 19 };

	ImVec2 UI::btnNormal{ UI::_btnNormal };
	ImVec2 UI::btnSmall{ UI::_btnSmall };
	ImVec2 UI::toolbarBtnSize{ UI::_toolbarBtnSize };
	ImVec2 UI::toolbarBtnImgSize{ UI::_toolbarBtnImgSize };
	char UI::idStr[256];

	std::vector<ImVec4> UI::accentColors
	{
		ImVec4{ 0.10f, 0.10f, 0.10f, 1.00f }, // User
		ImVec4{ 0.16f, 0.44f, 0.75f, 1.00f }, // Default
		ImVec4{ 0.30f, 0.31f, 0.86f, 1.00f }, // Light Music
		ImVec4{ 0.40f, 0.69f, 0.15f, 1.00f }, // Idol
		ImVec4{ 0.76f, 0.05f, 0.32f, 1.00f }, // Street
		ImVec4{ 0.81f, 0.45f, 0.06f, 1.00f }, // Theme Park
		ImVec4{ 0.50f, 0.25f, 0.55f, 1.00f }, // School Refusal
	};

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

		bool pressed = ImGui::ButtonEx(txt, size, (repeat ? ImGuiButtonFlags_Repeat : 0));

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

	bool UI::coloredButton(const char* txt, ImVec2 pos, ImVec2 size, ImU32 col, bool enabled)
	{
		ImVec4 col4 = ImGui::ColorConvertU32ToFloat4(col);
		ImVec4 colh = generateHighlightColor(col4);
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, !enabled);
		ImGui::PushStyleColor(ImGuiCol_Button, col4);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colh);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, colh);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.15f, 0.15f, 1.0));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

		ImGui::SetCursorScreenPos(pos);
		ImVec2 sz = size;
		if (sz.x <= 0)
			sz.x = std::max(ImGui::CalcTextSize(txt).x + 5.0f, 30.0f);

		if (sz.y <= 0)
			sz.y = ImGui::GetFrameHeightWithSpacing();

		sz += ImGui::GetStyle().FramePadding;

		bool pressed = ImGui::Button(txt, sz);

		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(1);
		ImGui::PopItemFlag();
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

	void UI::addPercentSliderProperty(const char* label, float& val)
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

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - UI::btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::InputTextWithHint(labelID(label), "n/a", &val, ImGuiInputTextFlags_EnterReturnsTrue))
			result = 1;
		ImGui::SameLine();

		ImGui::PushID(label);
		if (ImGui::Button("...", UI::btnSmall))
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

	void UI::addCheckboxProperty(const char* label, bool& val)
	{
		propertyLabel(label);

		ImGui::Checkbox(labelID(label), &val);
		ImGui::NextColumn();
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
		const char* id = labelID(label);
		ImGui::PushID(id);

		bool act = false;
		ImGui::SetNextItemWidth(100);
		if (ImGui::InputScalar("##custom_input", ImGuiDataType_S32, (void*)(&value),
			0, 0, IO::concat("%d", getString("division_suffix")).c_str()))
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

		size.x += ImGui::GetItemRectSize().x;
		size.y += size.y * 6;
		pos.y -= size.y;
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
		return act;
	}

	bool UI::zoomControl(const char* label, float& value, float min, float max)
	{
		bool act = false;
		if (UI::transparentButton(ICON_FA_SEARCH_MINUS, UI::btnSmall))
		{
			value -= 0.25f;
			act = true;
		}

		ImGui::SameLine();
		ImGui::SetNextItemWidth(120);

		act |= ImGui::SliderFloat(labelID(label), &value, min, max, "%.2fx");
		ImGui::SameLine();

		if (UI::transparentButton(ICON_FA_SEARCH_PLUS, UI::btnSmall))
		{
			value += 0.25f;
			act = true;
		}

		return act;
	}

	bool UI::timeSignatureSelect(int& numerator, int& denominator)
	{
		propertyLabel(getString("time_signature"));

		float controlWidth = (ImGui::GetContentRegionAvail().x / 2.0f) - (ImGui::CalcTextSize("/").x);
		bool edit = false;

		ImGui::SetNextItemWidth(controlWidth);
		if (ImGui::BeginCombo("##ts_num", std::to_string(numerator).c_str()))
		{
			for (int i = 1; i <= 32; ++i)
			{
				if (ImGui::Selectable(std::to_string(i).c_str()))
				{
					edit = numerator != i;
					numerator = i;
				}
			}

			ImGui::EndCombo();
		}

		ImGui::SameLine();
		ImGui::Text("/");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(controlWidth);

		if (ImGui::BeginCombo("##ts_denom", std::to_string(denominator).c_str()))
		{
			for (int i = 0; i < sizeof(timeSignatureDenominators) / sizeof(int); ++i)
			{
				if (ImGui::Selectable(std::to_string(timeSignatureDenominators[i]).c_str()))
				{
					edit = denominator != timeSignatureDenominators[i];
					denominator = timeSignatureDenominators[i];
				}
			}

			ImGui::EndCombo();
		}

		ImGui::NextColumn();
		return edit;
	}

	void UI::setWindowTitle(std::string title)
	{
		GLFWwindow* window = glfwGetCurrentContext();
		if (!window)
			return;

		glfwSetWindowTitle(window, std::string{ windowTitle }.append(title).c_str());
	}

	void UI::updateBtnSizesDpiScaling(float scale)
	{
		btnNormal = _btnNormal * scale;
		btnSmall = _btnSmall * scale;
		toolbarBtnSize = _toolbarBtnSize * scale;
		toolbarBtnImgSize = _toolbarBtnImgSize * scale;
	}

	bool UI::toolbarButton(const char* icon, const char* label, const char* shortcut, bool enabled, bool selected)
	{
		if (!enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * true));
		}

		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
		}

		bool activated = ImGui::Button(icon, UI::toolbarBtnSize);

		std::string tooltipLabel = label;
		if (shortcut && strlen(shortcut))
			tooltipLabel.append(" (").append(shortcut).append(")");

		if (tooltipLabel.size())
			tooltip(tooltipLabel.c_str());

		if (selected)
			ImGui::PopStyleColor(2);

		if (!enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		ImGui::SameLine();

		return activated;
	}

	bool UI::toolbarImageButton(const char* img, const char* label, const char* shortcut, bool enabled, bool selected)
	{
		std::string lblId;
		lblId.append("##").append(img).append(label);

		int texIndex = ResourceManager::getTexture(img);
		if (texIndex == -1)
		{
			// fallback to regular toolbar button
			return toolbarButton(img, label, shortcut, enabled, selected);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 2, 2 });
		if (!enabled)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1 - (0.5f * true));
		}

		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
		}

		bool activated = ImGui::ImageButton(lblId.c_str(), (void*)ResourceManager::textures[texIndex].getID(), UI::toolbarBtnImgSize);
		
		std::string tooltipLabel = label;
		if (shortcut && strlen(shortcut))
			tooltipLabel.append(" (").append(shortcut).append(")");

		if (tooltipLabel.size())
			tooltip(tooltipLabel.c_str());

		if (selected)
			ImGui::PopStyleColor(2);

		if (!enabled)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		ImGui::PopStyleVar();
		ImGui::SameLine();

		return activated;
	}

	void UI::toolbarSeparator()
	{
		ImGui::SameLine();
		ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
		ImGui::SameLine();
	}

	void UI::beginNextItemDisabled()
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
	}

	void UI::endNextItemDisabled()
	{
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}
}
