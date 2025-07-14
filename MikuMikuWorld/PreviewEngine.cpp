#include "PreviewEngine.h"
#include "Constants.h"
#include "ApplicationConfiguration.h"
#include <stack>
#include <stdexcept>

namespace MikuMikuWorld::Engine
{
	Range getNoteVisualTime(Note const& note, Score const& score, float noteSpeed) {
		float targetTime = accumulateScaledDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
		return {targetTime - getNoteDuration(noteSpeed), targetTime};
	}

	struct TargetTimeComparer {
		bool operator()(const Range& a, const Range& b) const
		{
			return a.max < b.max;
		}
	};

	void DrawData::calculateDrawData(Score const &score)
	{
		this->clear();
		try
		{
			this->noteSpeed = config.pvNoteSpeed;
			std::map<Range, Range, TargetTimeComparer> simBuilder;
			for (auto rit = score.notes.rbegin(), rend = score.notes.rend(); rit != rend; rit++) {
				auto& [id, note] = *rit;
				maxTicks = std::max(note.tick, maxTicks);
				NoteType type = note.getType();
				bool hidden = false;
				if (type == NoteType::Hold)
					hidden = score.holdNotes.at(id).startType != HoldNoteType::Normal;
				else if (type == NoteType::HoldEnd)
					hidden = score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal;
				if (!hidden && type != NoteType::HoldMid) {
					auto visual_tm = getNoteVisualTime(note, score, noteSpeed);
					drawingNotes.push_back(DrawingNote{note.ID, visual_tm});

					// Find the max and min lane within the same height (visual_tm.max)
					float center = getNoteCenter(note);
					auto&& [it, has_emplaced] = simBuilder.try_emplace(visual_tm, Range{center, center});
					auto& x_range = it->second;
					if (has_emplaced)
						continue;
					if (center < x_range.min)
						x_range.min = center;
					if (center > x_range.max)
						x_range.max = center;
					continue;
				}
			}

			for (const auto& [visual_tm, x_range] : simBuilder) {
				if (x_range.min != x_range.max)
					drawingLines.push_back(DrawingLine{x_range, visual_tm});
			}

			std::stack<HoldStep const*> skipStepIDs;
			float noteDuration = getNoteDuration(noteSpeed);
			for (auto rit = score.holdNotes.rbegin(), rend = score.holdNotes.rend(); rit != rend; rit++) {
				auto& [id, holdNote] = *rit;
				const Note& startNote = score.notes.at(holdNote.start.ID), endNote = score.notes.at(holdNote.end);
				float start_tm = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
				auto it = holdNote.steps.begin(), end = holdNote.steps.end();
				for (const HoldStep* head = &holdNote.start, *tail; head != nullptr; head = tail, ++it) {
					while (it != end && it->type == HoldStepType::Skip) {
						skipStepIDs.push(&(*it));
						++it;
					}
					tail = it != end ? &(*it) : nullptr;
					const Note& headNote = score.notes.at(head->ID), tailNote = score.notes.at(tail ? tail->ID : holdNote.end);
					float head_scaled_tm = accumulateScaledDuration(headNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
					float tail_scaled_tm = accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);

					drawingHoldSegments.push_back(DrawingHoldSegment {
						holdNote.end,
						headNote.ID,
						tailNote.ID,
						head_scaled_tm,
						tail_scaled_tm,
						start_tm,
						head->ease,
						holdNote.isGuide(),
					});

					while(skipStepIDs.size()) {
						const HoldStep& skipStep = *skipStepIDs.top();
						const Note& skipNote = score.notes.at(skipStep.ID);
						float step_scaled_tm = accumulateScaledDuration(skipNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
						auto ease = getEaseFunction(head->ease);
						float headLeft = Engine::laneToLeft(headNote.lane);
						float headRight = Engine::laneToLeft(headNote.lane) + headNote.width;
						float tailLeft = Engine::laneToLeft(tailNote.lane);
						float tailRight = Engine::laneToLeft(tailNote.lane) + tailNote.width;
						float stepT = unlerp(head_scaled_tm, tail_scaled_tm, step_scaled_tm);
						float stepLeft = ease(headLeft, tailLeft, stepT);
						float stepRight = ease(headRight, tailRight, stepT);
						drawingHoldTicks.push_back(DrawingHoldTick{
							skipStep.ID,
							stepLeft + (stepRight - stepLeft) / 2,
							{step_scaled_tm - noteDuration, step_scaled_tm}
						});
						skipStepIDs.pop();
					}
					if (tail && tail->type != HoldStepType::Hidden) {
						float tick_scaled_tm = accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
						drawingHoldTicks.push_back(DrawingHoldTick{
							tailNote.ID,
							getNoteCenter(tailNote),
							{tick_scaled_tm - noteDuration, tick_scaled_tm}
						});
					}
				}
			}
		}
		catch(const std::out_of_range& ex)
		{
			this->clear();
		}
	}

	void DrawData::clear() {
		drawingLines.clear();
		drawingNotes.clear();
		drawingHoldTicks.clear();
		drawingHoldSegments.clear();
		maxTicks = 1;
	}
}

