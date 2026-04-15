#pragma once
#include "Score.h"
#include <string>
#include <vector>

namespace MikuMikuWorld
{
	enum class ScoreDiagnosticSeverity : uint8_t
	{
		Warning,
		Crash
	};

	enum class ScoreDiagnosticCategory : uint8_t
	{
		TapOverlap,
		HoldCoverage,
		VisibleOverlap,
		HoldRelaySameTime,
		HoldNodeExactOverlap
	};

	struct ScoreDiagnostic
	{
		ScoreDiagnosticCategory category{};
		ScoreDiagnosticSeverity severity{};

		std::string code{};
		std::string title{};
		std::string summary{};
		std::string categoryText{};
		std::string positionText{};
		std::string trackText{};

		int measure{};
		int tick{};
		int lane{};
		int width{};
		int sourceNoteID{ -1 };
		int targetNoteID{ -1 };

		inline int getFocusNoteID() const
		{
			return targetNoteID > -1 ? targetNoteID : sourceNoteID;
		}
	};

	class ScoreDiagnostics
	{
	public:
		static std::vector<ScoreDiagnostic> analyze(const Score& score);
	};
}
