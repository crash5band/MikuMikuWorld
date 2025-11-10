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

	float Utilities::random(float min, float max)
	{
		if (min == max)
			return min;

		return min + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (max - min));
	}

	Random random{};

	float Random::get(float min, float max)
	{
		float factor = dist(gen);
		return lerp(min, max, factor);
	}

	float Random::get()
	{
		return dist(gen);
	}

	Color Utilities::random(const Color& min, const Color& max)
	{
		return Color(
			random(min.r, max.r),
			random(min.g, max.g),
			random(min.b, max.b),
			random(min.a, max.a)
		);
	}

	Quaternion Utilities::random(const Quaternion& min, const Quaternion& max)
	{
		return Quaternion(
			random(min.x, max.x),
			random(min.y, max.y),
			random(min.z, max.z),
			random(min.w, max.w)
		);
	}

	Vector3 Utilities::random(const Vector3& min, const Vector3& max)
	{
		if (min.x == min.y && min.y == min.z &&
			max.x == max.y && max.y == max.z)
		{
			float randomValue = random(min.x, max.x);
			return Vector3(randomValue, randomValue, randomValue);
		}

		return Vector3(
			random(min.x, max.x),
			random(min.y, max.y),
			random(min.z, max.z)
		);
	}

	void drawShadedText(ImDrawList* drawList, ImVec2 textPos, float fontSize, ImU32 fontColor, const char* text)
	{
		if (!drawList) return;

		drawList->AddText(ImGui::GetFont(), fontSize, textPos + ImVec2{ 0.75f, 1.0f }, 0xff111111, text);
		drawList->AddText(ImGui::GetFont(), fontSize, textPos, fontColor, text);
	}

	void drawTextureUnscaled(unsigned int texId, float texWidth, float texHeight, ImVec2 const &pos, ImVec2 const &size, ImU32 const &col)
	{
		const float source_ratio = texWidth / texHeight;
		const float target_ratio = size.x / size.y;
		ImVec2 target_size = {
			target_ratio >  source_ratio ? source_ratio * size.y : size.x,
			target_ratio >= source_ratio ? size.y : size.x / source_ratio
		};
		ImVec2 target_pos = pos + (size - target_size) / 2;
		ImGui::GetWindowDrawList()->AddImage((ImTextureID)(size_t)texId, target_pos, target_pos + target_size, {0, 0}, {1, 1}, col);
	}
}