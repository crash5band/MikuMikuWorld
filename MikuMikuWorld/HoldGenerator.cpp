#include "HoldGenerator.h"
#include "UI.h"
#include "Localization.h"
#include "Utilities.h"
#include "TimelineMode.h"
#include "Selection.h"
#include "Math.h"
#include "Constants.h"
#include <algorithm>

namespace MikuMikuWorld
{
	const static ImGuiTreeNodeFlags defaultOpen = ImGuiTreeNodeFlags_DefaultOpen;

	HoldGenerator::HoldGenerator() :
		stepType{ HoldStepType::Invisible },
		easeType{ EaseType::None },
		interpolation{ InterpolationMode::PositionAndSize },
		division{ 16 },
		laneOffset{ 3 },
		alignment{ Alignment::LeftRight },
		alignmentStart{ AlignmentStart::Left }
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

	int HoldGenerator::getStepPosition(int start, int end, float ratio, int width, bool left)
	{
		int lerpOffset = 0;
		if (interpolation == InterpolationMode::Position || interpolation == InterpolationMode::PositionAndSize)
		{
			lerpOffset = lerp(start, end, ratio) - start;
		}

		int leftFactor = 1;
		int rightFactor = 1;
		switch (alignment)
		{
		case Alignment::Left:
			leftFactor = 0;
			break;
		case Alignment::LeftRight:
			leftFactor = -1;
			break;
		case Alignment::Right:
			leftFactor = -1;
			rightFactor = 0;
			break;
		default:
			break;
		}

		lerpOffset += roundf(laneOffset * (left ? leftFactor : rightFactor));
		return std::clamp(start + lerpOffset, MIN_LANE, MAX_LANE - width);
	}

	bool HoldGenerator::updateWindow(const std::unordered_set<int>& selectedHolds)
	{
		bool canGen = selectedHolds.size();
		bool gen = false;

		if (ImGui::Begin("[WIP] Hold Generator"))
		{
			ImVec2 childSize = ImGui::GetContentRegionAvail();
			childSize.y -= ImGui::GetFrameHeightWithSpacing() + (ImGui::GetStyle().WindowPadding.y * 2);

			if (ImGui::BeginChild("hold_gen_options", childSize, true))
			{
				if (ImGui::CollapsingHeader("Basic Settings", defaultOpen))
				{
					UI::beginPropertyColumns();
					UI::divisionSelect(getString("division"), division, divisions, sizeof(divisions) / sizeof(int));
					UI::addSelectProperty(getString("step_type"), stepType, uiStepTypes, TXT_ARR_SZ(uiStepTypes));
					UI::addSelectProperty(getString("ease_type"), easeType, uiEaseTypes, TXT_ARR_SZ(uiEaseTypes));
					UI::addIntProperty("Lane Offset", laneOffset, 0, 11);
					UI::endPropertyColumns();
				}

				if (ImGui::CollapsingHeader("Alignment", defaultOpen))
				{
					ImGui::SetCursorPosX(Utilities::centerImGuiItem(UI::btnNormal.x * 6));
					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));

					for (int i = 0; i < 3; ++i)
					{
						const bool selected = (int)alignment == i;
						if (selected)
						{
							// highlight selected mode
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyle().Colors[ImGuiCol_TabActive]);
						}

						if (ImGui::Button(alignmentModes[i], ImVec2(UI::btnNormal.x * 2, UI::btnNormal.y)))
							alignment = (Alignment)i;

						if (selected)
							ImGui::PopStyleColor(2);

						if (i < 2)
							ImGui::SameLine();
					}

					ImGui::PopStyleVar();

					ImGui::Text("Alignment Starting Position");
					ImGui::RadioButton(alignmentStarts[0], (int*)&alignmentStart, 0);
					ImGui::RadioButton(alignmentStarts[1], (int*)&alignmentStart, 1);

					UI::beginPropertyColumns();
					UI::addSelectProperty("Interpolation", interpolation, interpolationModes, TXT_ARR_SZ(interpolationModes));
					UI::endPropertyColumns();
				}
			}
			ImGui::EndChild();

			ImGui::Separator();
			if (ImGui::Button("Generate", ImVec2(-1, UI::btnSmall.y + 2.0f)))
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
			hold.start.ease = easeType;

			const Note& start = score.notes.at(hold.start.ID);
			const Note& end = score.notes.at(hold.end);

			bool left = alignmentStart == AlignmentStart::Left;
			for (int t = start.tick + interval; t < end.tick; t += interval)
			{
				float ratio = (float)(t - start.tick) / (float)(end.tick - start.tick);
				Note mid{ NoteType::HoldMid };
				mid.ID = nextID++;
				mid.parentID = start.ID;
				mid.critical = start.critical;
				mid.tick = t;
				mid.width = getStepWidth(start.width, end.width, ratio);
				mid.lane = getStepPosition(start.lane, end.lane, ratio, mid.width, left);

				HoldStep step{ mid.ID, stepType, easeType };
				score.notes[mid.ID] = mid;
				hold.steps.push_back(step);

				left ^= true;
			}
		}
	}
}