#include "UI.h"
#include <string>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <Windows.h>

namespace MikuMikuWorld
{
	const ImVec2 UI::btnNormal{ 30, 30 };
	const ImVec2 UI::btnSmall{ 22, 22 };

	std::vector<AccentColor> UI::accentColors{
		AccentColor{ "User",			ImVec4(0.10f, 0.10f, 0.10f, 1.00f) },
		AccentColor{ "Default",			ImVec4(0.38f, 0.37f, 0.82f, 1.00f) },
		AccentColor{ "MikuMikuWorld",	ImVec4(0.19f, 0.75f, 0.62f, 1.00f) },
		AccentColor{ "Light Music",		ImVec4(0.30f, 0.31f, 0.86f, 1.00f) },
		AccentColor{ "Idol", 			ImVec4(0.40f, 0.69f, 0.15f, 1.00f) },
		AccentColor{ "Street", 			ImVec4(0.76f, 0.05f, 0.32f, 1.00f) },
		AccentColor{ "Theme Park", 		ImVec4(0.81f, 0.45f, 0.06f, 1.00f) },
		AccentColor{ "School Refusal",	ImVec4(0.50f, 0.25f, 0.55f, 1.00f) },
		AccentColor{ "Plain",			ImVec4(0.40f, 0.40f, 0.40f, 1.00f) }
	};

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

		std::string id{ "##" };
		id.append(txt);

		ImGui::SetCursorScreenPos(pos);
		bool pressed = ImGui::Button(id.c_str(), size);

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

		std::string id("##");
		id.append(label);

		ImGui::InputText(id.c_str(), &val);
		ImGui::NextColumn();
	}

	void UI::addIntProperty(const char* label, int& val, int lowerBound, int higherBound)
	{
		propertyLabel(label);
		
		std::string id("##");
		id.append(label);

		ImGui::InputInt(id.c_str(), &val, 1, 5);
		if (lowerBound != higherBound)
			val = std::clamp(val, lowerBound, higherBound);
		ImGui::NextColumn();
	}

	void UI::addFloatProperty(const char* label, float& val, const char* format)
	{
		propertyLabel(label);
		
		std::string id("##");
		id.append(label);

		ImGui::InputFloat(id.c_str(), &val, 1.0f, 10.f, format);
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

		std::string id("##");
		id.append(label);

		ImGui::SliderInt(id.c_str(), &val, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();
	}

	void UI::addSliderProperty(const char* label, float& val, float min, float max, const char* format)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		ImGui::SliderFloat(id.c_str(), &val, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();
	}

	void UI::addPercentSliderProperty(const char* label, float &val)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		float scaled = val * 100;
		ImGui::SliderFloat(id.c_str(), &scaled, 0, 100, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();

		val = scaled / 100.0f;
	}

	int UI::addFileProperty(const char* label, std::string& val)
	{
		propertyLabel(label);
		
		std::string id("##");
		id.append(label);

		int result = 0;

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
		if (ImGui::InputTextWithHint(id.c_str(), "n/a", &val, ImGuiInputTextFlags_EnterReturnsTrue))
			result = 1;
		ImGui::SameLine();

		if (ImGui::Button("...", btnSmall))
			result = 2;

		ImGui::NextColumn();

		return result;
	}

	bool UI::addFractionProperty(const char* label, int& numerator, int& denominator)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		std::string idNumerator = id + "_numerator";
		std::string idDenominator = id + "_denominator";

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

		std::string id("##");
		id.append(label);

		ImGui::InputTextMultiline(id.c_str(), &val, ImVec2(-1, 50));
		ImGui::NextColumn();
	}

	void UI::setWindowTitle(const std::string& title)
	{
		GLFWwindow* window = glfwGetCurrentContext();
		if (!window)
			return;

		std::string fullTitle = windowTitle + title;
		glfwSetWindowTitle(window, fullTitle.c_str());
	}
}
