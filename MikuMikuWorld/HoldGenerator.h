#pragma once
#include <unordered_set>
#include "NoteTypes.h"

namespace MikuMikuWorld
{
	struct Score;
	class Selection;

	enum class InterpolationMode
	{
		None,
		Position,
		Size,
		PositionAndSize
	};

	constexpr const char* interpolationModes[] = {
		"None",
		"Position",
		"Size",
		"Position & Size"
	};

	class HoldGenerator
	{
	private:
		HoldStepType stepType;
		int laneOffset;
		int division;

		int lerpRadioGroup;
		int alignRadioGroup;

		InterpolationMode interpolation;

		int getStepWidth(int start, int end, float ratio);
		int getStepPosition(int start, int end, float ratio);

	public:
		HoldGenerator();

		bool updateWindow(const std::unordered_set<int>& selectedHolds);
		void generate(Score& score, Selection sel);
	};
}