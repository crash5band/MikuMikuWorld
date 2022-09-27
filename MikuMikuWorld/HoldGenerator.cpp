#include "HoldGenerator.h"
#include "UI.h"
#include "Localization.h"
#include "Utilities.h"
#include "TimelineMode.h"
#include "Selection.h"
#include "Math.h"

namespace MikuMikuWorld
{
	const static ImGuiTreeNodeFlags defaultOpen = ImGuiTreeNodeFlags_DefaultOpen;

	HoldGenerator::HoldGenerator() :
		stepType{ HoldStepType::Invisible },
		interpolation{ InterpolationMode::PositionAndSize },
		division{ 16 },
		laneOffset{ 3 },
		alignRadioGroup{ 1 }
	{

	}

	int HoldGenerator::getStepWidth(int start, int end, float ratio)
	{
		if (interpolation == InterpolationMode::Size || interpolation == InterpolationMode::PositionAndSize)
		{
			return lerp(start, end, ratio);
		}
		else
		{
			return start;
		}
	}

	int HoldGenerator::getStepPosition(int start, int end, float ratio)
	{
		if (interpolation == InterpolationMode::Position || interpolation == InterpolationMode::PositionAndSize)
		{
			return lerp(start, end, ratio);
		}
		else
		{
			return start;
		}
	}

	bool HoldGenerator::updateWindow(const std::unordered_set<int>& selectedHolds)
	{
		bool canGen = selectedHolds.size();
		bool gen = false;

		if (ImGui::Begin("[WIP] Hold Generator"))
		{
			ImVec2 childSize = ImGui::GetContentRegionAvail();
			childSize.y -= ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y;

			if (ImGui::BeginChild("hold_gen_options", childSize, true))
			{
				if (ImGui::CollapsingHeader("Basic Settings", defaultOpen))
				{
					UI::beginPropertyColumns();
					UI::divisionSelect(getString("division"), division, divisions, sizeof(divisions) / sizeof(int));
					UI::addSelectProperty(getString("step_type"), stepType, uiStepTypes, TXT_ARR_SZ(uiStepTypes));
					UI::addIntProperty("Lane Offset", laneOffset);
					UI::endPropertyColumns();
				}

				ImGui::Separator();
				ImGui::RadioButton("L", &alignRadioGroup, 0);
				ImGui::RadioButton("R", &alignRadioGroup, 3);
				ImGui::RadioButton("L <-> R", &alignRadioGroup, 1);
				ImGui::RadioButton("R <-> L", &alignRadioGroup, 2);

				if (ImGui::CollapsingHeader("Advanced Settings", defaultOpen))
				{
					UI::beginPropertyColumns();
					UI::addSelectProperty("Interpolation", interpolation, interpolationModes, TXT_ARR_SZ(interpolationModes));
					UI::endPropertyColumns();
				}
			}
			ImGui::EndChild();

			ImGui::Separator();
			if (ImGui::Button("Generate", ImVec2(-1, 0)))
				gen = true;
		}

		ImGui::End();
		return gen;
	}

	void HoldGenerator::generate(Score& score, Selection sel)
	{
		if (!sel.hasSelection())
			return;

		auto holdIDs = sel.getHolds(score);
		int interval = 1920 / division;
		for (int id : holdIDs)
		{
			// remove old steps
			HoldNote& hold = score.holdNotes.at(id);
			for (const auto& step : hold.steps)
				score.notes.erase(step.ID);
			hold.steps.clear();

			const Note& start = score.notes.at(hold.start.ID);
			const Note& end = score.notes.at(hold.end);

			bool left = alignRadioGroup >= 2 ? true : false;
			for (int t = start.tick + interval; t < end.tick; t += interval)
			{
				float ratio = (float)(t - start.tick) / (float)(end.tick - start.tick);
				Note mid{ NoteType::HoldMid };
				mid.ID = nextID++;
				mid.parentID = start.ID;
				mid.critical = start.critical;
				mid.tick = t;
				mid.width = getStepWidth(start.width, end.width, ratio);
				mid.lane = getStepPosition(start.lane, end.lane, ratio);

				if (alignRadioGroup == 0)
				{
					// left aligned
					mid.lane = roundf(start.lane + (laneOffset * (left ? 0 : 1)));
				}
				else if (alignRadioGroup == 1 || alignRadioGroup == 2)
				{
					// alternate left and right
					mid.lane = roundf(start.lane + (laneOffset * (left ? -1 : 1)));
				}
				else if (alignRadioGroup == 3)
				{
					// left aligned
					mid.lane = roundf(start.lane + (laneOffset * (left ? -1 : 0)));
				}

				HoldStep step{ mid.ID, stepType, EaseType::None };
				score.notes[mid.ID] = mid;
				hold.steps.push_back(step);

				left ^= true;
			}
		}
	}
}