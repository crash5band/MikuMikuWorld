#include "UI.h"
#include <string>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <Windows.h>

namespace MikuMikuWorld
{
	std::vector<AccentColor> accentColors{
		AccentColor{ "Default", ImVec4(0.38f, 0.37f, 0.82f, 1.00f) },
		AccentColor{ "MikuMikuWorld", ImVec4(0.19f, 0.75f, 0.62f, 1.00f) },
		AccentColor{ "Light Music", ImVec4(0.30f, 0.31f, 0.86f, 1.00f), },
		AccentColor{ "Idol", 		ImVec4(0.40f, 0.69f, 0.15f, 1.00f), },
		AccentColor{ "Street", 		ImVec4(0.76f, 0.05f, 0.32f, 1.00f), },
		AccentColor{ "Theme Park", 	ImVec4(0.81f, 0.45f, 0.06f, 1.00f), },
		AccentColor{ "School Refusal", ImVec4(0.50f, 0.25f, 0.55f, 1.00f) },
		AccentColor{ "Plain", ImVec4(0.40f, 0.40f, 0.40f, 1.00f) }
	};

	bool transparentButton(const char* txt, ImVec2 size, bool repeat)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushButtonRepeat(repeat);
		bool pressed = ImGui::Button(txt, size);
		ImGui::PopButtonRepeat();

		ImGui::PopStyleColor();
		return pressed;
	}

	bool transparentButton2(const char* txt, ImVec2 pos, ImVec2 size)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.4f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.4f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

		std::string id{ "##" };
		id.append(txt);

		ImGui::SetCursorPos(pos);
		bool pressed = ImGui::Button(id.c_str(), size);

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		return pressed;
	}

	bool isAnyPopupOpen()
	{
		return ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
	}

	void beginPropertyColumns()
	{
		ImGui::Columns(2);
	}

	void endPropertyColumns()
	{
		ImGui::Columns(1);
	}

	void propertyLabel(const char* label)
	{
		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::SetNextItemWidth(-1);
	}

	void addStringProperty(const char* label, std::string& val)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		ImGui::InputText(id.c_str(), &val);
		ImGui::NextColumn();
	}

	void addIntProperty(const char* label, int& val, int lowerBound, int higherBound)
	{
		propertyLabel(label);
		
		std::string id("##");
		id.append(label);

		ImGui::InputInt(id.c_str(), &val, 1, 5);
		if (lowerBound != higherBound)
			val = std::clamp(val, lowerBound, higherBound);
		ImGui::NextColumn();
	}

	void addFloatProperty(const char* label, float& val, const char* format)
	{
		propertyLabel(label);
		
		std::string id("##");
		id.append(label);

		ImGui::InputFloat(id.c_str(), &val, 0.01f, 0.1f, format);
		ImGui::NextColumn();
	}

	void addReadOnlyProperty(const char* label, std::string val)
	{
		propertyLabel(label);
		ImGui::Text(val.c_str());
		ImGui::NextColumn();
	}

	void addSliderProperty(const char* label, int& val, int min, int max, const char* format)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		ImGui::SliderInt(id.c_str(), &val, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();
	}

	void addSliderProperty(const char* label, float& val, float min, float max, const char* format)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		ImGui::SliderFloat(id.c_str(), &val, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();
	}

	void addPercentSliderProperty(const char* label, float &val)
	{
		propertyLabel(label);

		std::string id("##");
		id.append(label);

		float scaled = val * 100;
		ImGui::SliderFloat(id.c_str(), &scaled, 0, 100, "%.0f%%", ImGuiSliderFlags_AlwaysClamp);
		ImGui::NextColumn();

		val = scaled / 100.0f;
	}

	bool addFileProperty(const char* label, std::string val)
	{
		propertyLabel(label);
		
		std::string id("##");
		id.append(label);

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnSmall.x - ImGui::GetStyle().ItemSpacing.x);
		ImGui::InputText(id.c_str(), &val, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();

		bool pressed = ImGui::Button("...", btnSmall);
		ImGui::NextColumn();

		return pressed;
	}

	bool addFractionProperty(const char* label, int& numerator, int& denominator)
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

	void setWindowTitle(const std::string& title)
	{
		GLFWwindow* window = glfwGetCurrentContext();
		if (!window)
			return;

		glfwSetWindowTitle(window, title.c_str());
	}
}