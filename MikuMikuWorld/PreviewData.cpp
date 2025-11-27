#include <queue>
#include <stdexcept>
#include "PreviewData.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "ResourceManager.h"
#include "ScoreContext.h"

namespace MikuMikuWorld::Engine
{
	struct DrawingHoldStep
	{
		int tick;
		double time;
		float left;
		float right;
		EaseType ease;
	};

	static void addHoldNote(DrawData& drawData, const HoldNote& holdNote, Score const &score);

	void DrawData::calculateDrawData(Score const &score)
	{
		this->clear();
		try
		{
			this->noteSpeed = config.pvNoteSpeed;
			std::map<int, Range> simBuilder;
			for (auto rit = score.notes.rbegin(), rend = score.notes.rend(); rit != rend; rit++)
			{
				auto& [id, note] = *rit;
				maxTicks = std::max(note.tick, maxTicks);
				NoteType type = note.getType();
				if (type == NoteType::HoldMid
					|| (type == NoteType::Hold && score.holdNotes.at(id).startType != HoldNoteType::Normal)
					|| (type == NoteType::HoldEnd && score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal))
					continue;
				if (type == NoteType::HoldMid)
					continue;
					
				auto visual_tm = getNoteVisualTime(note, score, noteSpeed);
				drawingNotes.push_back(DrawingNote{note.ID, visual_tm});

				// Find the max and min lane within the same height (visual_tm.max)
				float center = getNoteCenter(note);
				auto&& [it, has_emplaced] = simBuilder.try_emplace(note.tick, Range{center, center});
				auto& x_range = it->second;
				if (has_emplaced)
					continue;
				if (center < x_range.min)
					x_range.min = center;
				if (center > x_range.max)
					x_range.max = center;
				continue;
			}

			float noteDuration = getNoteDuration(noteSpeed);
			for (const auto& [line_tick, x_range] : simBuilder)
			{
				if (x_range.min != x_range.max)
				{
					double targetTime = accumulateScaledDuration(line_tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
					drawingLines.push_back(DrawingLine{ x_range, Range{ targetTime - getNoteDuration(noteSpeed), targetTime } });
				}
			}

			for (auto rit = score.holdNotes.rbegin(), rend = score.holdNotes.rend(); rit != rend; rit++)
			{
				addHoldNote(*this, rit->second, score);
			}
		}
		catch(const std::out_of_range& ex)
		{
			this->clear();
		}
	}

	void DrawData::clear()
	{
		drawingLines.clear();
		drawingNotes.clear();
		drawingHoldTicks.clear();
		drawingHoldSegments.clear();
		effectView.reset();

		maxTicks = 1;
	}

	void addHoldNote(DrawData &drawData, const HoldNote &holdNote, Score const &score)
	{
		float noteDuration = getNoteDuration(drawData.noteSpeed);
		const Note& startNote = score.notes.at(holdNote.start.ID), endNote = score.notes.at(holdNote.end);
		float activeTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float startTime = activeTime;
		DrawingHoldStep head = {
			startNote.tick,
			accumulateScaledDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
			Engine::laneToLeft(startNote.lane),
			Engine::laneToLeft(startNote.lane) + startNote.width,
			holdNote.start.ease
		};
		for (ptrdiff_t headIdx = -1, tailIdx = 0, stepSz = holdNote.steps.size(); headIdx < stepSz; ++tailIdx)
		{
			if (tailIdx < stepSz && holdNote.steps[tailIdx].type == HoldStepType::Skip)
				continue;
			HoldStep tailStep = tailIdx == stepSz ? HoldStep{ holdNote.end, HoldStepType::Hidden } : holdNote.steps[tailIdx];
			const Note& tailNote = score.notes.at(tailStep.ID);
			auto easeFunction = getEaseFunction(head.ease);
			DrawingHoldStep tail = {
				tailNote.tick,
				accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges),
				Engine::laneToLeft(tailNote.lane),
				Engine::laneToLeft(tailNote.lane) + tailNote.width,
				tailStep.ease
			};
			float endTime = accumulateDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges);
			drawData.drawingHoldSegments.push_back(DrawingHoldSegment {
				holdNote.end, 
				head.ease,
				holdNote.isGuide(),
				tailIdx,
				head.time, tail.time,
				head.left, head.right,
				tail.left, tail.right,
				startTime, endTime,
				activeTime,
			});
			startTime = endTime;
			while ((headIdx + 1) < tailIdx)
			{
				const HoldStep& skipStep = holdNote.steps[headIdx + 1];
				assert(skipStep.type == HoldStepType::Skip);
				const Note& skipNote = score.notes.at(skipStep.ID);
				if (skipNote.tick > tail.tick)
					break;
				double tickTime = accumulateScaledDuration(skipNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
				double tick_t = unlerpD(head.time, tail.time, tickTime);
				float skipLeft = easeFunction(head.left, tail.left, tick_t);
				float skipRight = easeFunction(head.right, tail.right, tick_t);
				drawData.drawingHoldTicks.push_back(DrawingHoldTick{
					skipStep.ID,
					skipLeft + (skipRight - skipLeft) / 2,
					Range{tickTime - noteDuration, tickTime}
				});
				++headIdx;
			}
			if (tailStep.type != HoldStepType::Hidden)
			{
				double tickTime = accumulateScaledDuration(tailNote.tick, TICKS_PER_BEAT, score.tempoChanges, score.hiSpeedChanges);
				drawData.drawingHoldTicks.push_back(DrawingHoldTick{
					tailNote.ID,
					getNoteCenter(tailNote),
					{tickTime - noteDuration, tickTime}
				});
			}
			head = tail;
			++headIdx;
		}
	}
}
