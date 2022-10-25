#pragma once
#include "ImGui/imgui.h"
#include "Color.h"

namespace MikuMikuWorld
{
	const ImU32 measureColor	= ImGui::ColorConvertFloat4ToU32(ImVec4(0.80f, 0.80f, 0.80f, 0.70f));
	const ImU32 measureTxtColor	= ImGui::ColorConvertFloat4ToU32(ImVec4(0.80f, 0.80f, 0.80f, 1.00f));
	const ImU32 divColor1		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.60f, 0.60f, 0.60f, 0.70f));
	const ImU32 divColor2		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.35f, 0.35f, 0.50f));
	const ImU32 cursorColor		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.87f, 0.23f, 0.23f, 1.00f));
	const ImU32 tempoColor		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.52f, 0.93f, 0.35f, 0.85f));
	const ImU32 timeColor		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.99f, 0.82f, 0.22f, 0.85f));
	const ImU32 skillColor		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.20f, 0.58f, 1.00f, 0.85f));
	const ImU32 feverColor		= ImGui::ColorConvertFloat4ToU32(ImVec4(0.90f, 0.54f, 0.28f, 0.85f));
	const ImU32 selectionColor1 = ImGui::ColorConvertFloat4ToU32(ImVec4(0.40f, 0.40f, 0.40f, 0.45f));
	const ImU32 selectionColor2 = ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.46f, 1.00f, 0.50f));
	const ImU32 selectionShadow = ImGui::ColorConvertFloat4ToU32(ImVec4(0.20f, 0.20f, 0.20f, 0.65f));
	const ImU32 warningColor	= ImGui::ColorConvertFloat4ToU32(ImVec4(0.96f, 0.26f, 0.21f, 0.50f));
	const ImU32 bgFallbackColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.10f, 0.10f, 1.00f));

	const Color noteTint{ 1.0f, 1.0f, 1.0f, 1.0f };
	const Color hoverTint{ 1.0f, 1.0f, 1.0f, 0.70f };

	static ImVec4 generateDarkColor(const ImVec4& color)
	{
		return ImVec4(
			std::max(0.0f, color.x - 0.10f),
			std::max(0.0f, color.y - 0.10f),
			std::max(0.0f, color.z - 0.10f),
			1.0f);
	}

	static ImVec4 generateHighlightColor(const ImVec4& color)
	{
		return ImVec4(
			std::max(0.0f, color.x + 0.10f),
			std::max(0.0f, color.y + 0.10f),
			std::max(0.0f, color.z + 0.10f),
			1.0f);
	}
}