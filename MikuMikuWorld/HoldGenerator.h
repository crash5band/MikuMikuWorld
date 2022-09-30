#pragma once
#include <unordered_set>
#include "NoteTypes.h"
#include "IconsFontAwesome5.h"

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

	enum class Alignment
	{
		Left,
		LeftRight,
		Right
	};

	enum class AlignmentStart
	{
		Left,
		Right
	};

	enum class CurveType
	{
		Normal,
		ShrinkGrow,
		Wavy
	};

	constexpr const char* interpolationModes[] =
	{
		"None",
		"Position",
		"Size",
		"Position & Size"
	};

	constexpr const char* alignmentModes[] =
	{
		ICON_FA_ALIGN_LEFT,
		ICON_FA_ALIGN_CENTER,
		ICON_FA_ALIGN_RIGHT
	};

	constexpr const char* alignmentStarts[] =
	{
		"Left",
		"Right"
	};

	constexpr const char* curveTypes[] =
	{
		"Normal",
		"ShrinkGrow",
		"Wavy"
	};

	class HoldGenerator
	{
	private:
		HoldStepType stepType;
		EaseType easeType;
		int laneOffset;
		int division;
		InterpolationMode interpolation;
		Alignment alignment;
		AlignmentStart alignmentStart;

		int getStepWidth(int start, int end, float ratio);
		int getStepPosition(int start, int end, float ratio, int width, bool left);

	public:
		HoldGenerator();

		bool updateWindow(const std::unordered_set<int>& selectedHolds);
		void generate(Score& score, Selection sel);
	};
}