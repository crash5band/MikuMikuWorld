#include "Utilities.h"
#include "Constants.h"
#include "ImGui/imgui.h"

namespace MikuMikuWorld
{
	float Utilities::centerImGuiItem(const float width)
	{
		return (ImGui::GetContentRegionAvail().x * 0.5f) - (width * 0.5f);
	}

	void Utilities::ImGuiCenteredText(const std::string& str)
	{
		Utilities::ImGuiCenteredText(str.c_str());
	}

	void Utilities::ImGuiCenteredText(const char* str)
	{
		ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(str).x) * 0.5f);
		ImGui::Text(str);
	}
}