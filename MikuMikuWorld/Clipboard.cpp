#include "Clipboard.h"
#include "ImGui/imgui.h"
#include "IO.h"
#include <map>

namespace MikuMikuWorld
{
	static constexpr const char* clipboardSignatureLF = "MikuMikuWorld clipboard\n";
	static constexpr const char* clipboardSignatureCRLF = "MikuMikuWorld clipboard\r\n";

	static std::map<std::string, EaseType> stringToEaseTypeMap =
	{
		{"linear", EaseType::Linear},
		{"ease_in", EaseType::EaseIn},
		{"ease_out", EaseType::EaseOut},
		{"in", EaseType::EaseIn},
		{"out", EaseType::EaseOut}
	};

	static std::map<std::string, HoldStepType> stringToHoldStepTypeMap =
	{
		{"normal", HoldStepType::Normal},
		{"hidden", HoldStepType::Hidden},
		{"skip", HoldStepType::Skip},
		{"invisible", HoldStepType::Hidden},
		{"ignored", HoldStepType::Skip}
	};

	std::string_view Clipboard::get()
	{
		const char* clipboardDataPtr = ImGui::GetClipboardText();
		if (clipboardDataPtr == nullptr)
			return {};

		std::string_view content{ clipboardDataPtr };
		if (!IO::startsWith(content, clipboardSignatureLF) && !IO::startsWith(content, clipboardSignatureCRLF))
			return {};

		size_t dataOffset = content.find_first_of('\n', 0);
		return content.substr(dataOffset);
	}

	void Clipboard::store(const std::string& data)
	{
		ImGui::SetClipboardText(std::string(clipboardSignatureLF).append(data).c_str());
	}

	EaseType Clipboard::stringToEaseType(const std::string& str, EaseType def)
	{
		auto it = stringToEaseTypeMap.find(str);
		if (it == stringToEaseTypeMap.end())
			return def;

		return it->second;
	}

	HoldStepType Clipboard::stringToHoldStepType(const std::string& str, HoldStepType def)
	{
		auto it = stringToHoldStepTypeMap.find(str);
		if (it == stringToHoldStepTypeMap.end())
			return def;

		return it->second;
	}
}