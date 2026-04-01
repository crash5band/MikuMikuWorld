#include <queue>
#include <stdexcept>
#include "PreviewData.h"
#include "ApplicationConfiguration.h"
#include "Constants.h"
#include "ResourceManager.h"

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
				switch (type)
				{
				case NoteType::Hold:
					notesList.add(score.holdNotes.at(id), score);
					break;
				default:
					notesList.add(note, score);
				}

				if (type == NoteType::HoldMid ||
					type == NoteType::Hold && score.holdNotes.at(id).startType != HoldNoteType::Normal ||
					(type == NoteType::HoldEnd && score.holdNotes.at(note.parentID).endType != HoldNoteType::Normal))
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

			notesList.explicitSort();
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

		notesList.clear();
		effectView.reset();

		maxTicks = 1;
	}

	void addHoldNote(DrawData &drawData, const HoldNote &holdNote, Score const &score)
	{
		float noteDuration = getNoteDuration(drawData.noteSpeed);
		const Note& startNote = score.notes.at(holdNote.start.ID), endNote = score.notes.at(holdNote.end);
		float activeTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float startTime = activeTime;
		float endTime = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);

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

	void SortedDrawingNotesList::add(DrawingNoteTime note)
	{
		notes.push_back(note);
	}

	void SortedDrawingNotesList::add(const Note& note, const Score& score)
	{
		float time = accumulateDuration(note.tick, TICKS_PER_BEAT, score.tempoChanges);
		notes.push_back({note.ID, note.tick, note.tick, note.lane, time, time});
	}

	void SortedDrawingNotesList::add(const HoldNote& hold, const Score& score)
	{
		const Note& startNote = score.notes.at(hold.start.ID);
		const Note& endNote = score.notes.at(hold.end);

		float startTime = accumulateDuration(startNote.tick, TICKS_PER_BEAT, score.tempoChanges);
		float endTime = accumulateDuration(endNote.tick, TICKS_PER_BEAT, score.tempoChanges);

		notes.push_back({ startNote.ID, startNote.tick, endNote.tick, startNote.lane, startTime, endTime });
	}

	void SortedDrawingNotesList::clear()
	{
		notes.clear();
	}

	void SortedDrawingNotesList::explicitSort()
	{
		std::sort(notes.begin(), notes.end(), [](const auto& a, const auto& b)
		{
			return a.tick == b.tick ? a.lane < b.lane : a.tick < b.tick;
		});
	}

	void SortedDrawingNotesList::reserve(size_t capacity)
	{
		notes.reserve(capacity);
	}

	const std::vector<DrawingNoteTime>& SortedDrawingNotesList::getView() const
	{
		return notes;
	}

	std::vector<int> SortedDrawingNotesList::getTickRange(int from, int to) const
	{
		// TODO: Improve this such that we don't have to backtrack through all of the notes
		// to find hold notes in range
		auto cutoff = std::upper_bound(notes.begin(), notes.end(), to, [](int val, const auto& note) { return val < note.tick; });

		std::vector<int> result;
		result.reserve(std::distance(notes.begin(), cutoff) + 1);

		for (auto it = notes.begin(); it != cutoff; ++it)
		{
			if (it->endTick >= from)
				result.push_back(std::distance(notes.begin(), it));
		}

		return result;

		//return { binarySearch(from), binarySearch(to) };
	}

	int SortedDrawingNotesList::binarySearch(int targetTick) const
	{
		int l = 0, h = notes.size() - 1;
		int m = l + (h - l) / 2;
		
		while (l < h)
		{
			int middleTick = notes.at(m).tick;

			if (middleTick < targetTick)
				l = m + 1;
			else if (middleTick > targetTick)
				h = m - 1;
			else
				return m;

			m = l + (h - l) / 2;
		}

		return m;
	}
}
