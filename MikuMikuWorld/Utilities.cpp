#include "Utilities.h"
#include "Constants.h"
#include "ImGui/imgui.h"
#include "Localization.h"
#include "IO.h"
#include <Windows.h>
#include <ctime>

namespace MikuMikuWorld
{
	std::string Utilities::getCurrentDateTime()
	{
		std::time_t now = std::time(0);
		std::tm localTime = *std::localtime(&now);

		char buf[128];
		strftime(buf, 128, "%Y-%m-%d-%H-%M-%S", &localTime);

		return buf;
	}

	std::string Utilities::getSystemLocale()
	{
		LPWSTR lpLocalName = new WCHAR[LOCALE_NAME_MAX_LENGTH];
		int result = GetUserDefaultLocaleName(lpLocalName, LOCALE_NAME_MAX_LENGTH);

		std::wstring wL = lpLocalName;
		wL = wL.substr(0, wL.find_first_of(L"-"));

		delete[] lpLocalName;
		return IO::wideStringToMb(wL);
	}

	std::string Utilities::getDivisionString(int div)
	{
		return std::to_string(div) + getString("division_suffix");
	}

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

	void drawShadedText(ImDrawList* drawList, ImVec2 textPos, float fontSize, ImU32 fontColor, const char* text)
	{
		if (!drawList) return;

		drawList->AddText(ImGui::GetFont(), fontSize, textPos + ImVec2{ 1.5f, 1.5f }, 0xff111111, text);
		drawList->AddText(ImGui::GetFont(), fontSize, textPos, fontColor, text);
	}
}