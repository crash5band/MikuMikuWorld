#pragma once
#include "NoteTypes.h"
#include <string>

namespace MikuMikuWorld
{
	class Clipboard
	{
	public:
		static std::string_view get();
		static void store(const std::string& data);

		static EaseType stringToEaseType(const std::string&, EaseType def = EaseType::Linear);
		static HoldStepType stringToHoldStepType(const std::string&, HoldStepType def = HoldStepType::Normal);
	};
}