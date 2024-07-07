#include "ScoreStats.h"
#include "Score.h"
#include "Constants.h"
#include <algorithm>

namespace MikuMikuWorld
{
	ScoreStats::ScoreStats() { reset(); }

	void ScoreStats::reset()
	{
		resetCounts();
		resetCombo();
	}

	void ScoreStats::resetCounts() { taps = flicks = holds = steps = traces = total = 0; }

	void ScoreStats::resetCombo() { combo = 0; }

	void ScoreStats::calculateStats(const Score& score)
	{
		taps = std::count_if(score.notes.begin(), score.notes.end(),
		                     [](const auto& n)
		                     {
			                     const Note& note = n.second;
			                     return note.getType() == NoteType::Tap && !note.isFlick() &&
			                            !note.friction;
		                     });

		holds = std::count_if(score.notes.begin(), score.notes.end(),
		                      [](const auto& n) { return n.second.getType() == NoteType::Hold; });

		steps =
		    std::count_if(score.notes.begin(), score.notes.end(),
		                  [](const auto& n) { return n.second.getType() == NoteType::HoldMid; });

		flicks = std::count_if(score.notes.begin(), score.notes.end(),
		                       [](const auto& n) { return n.second.isFlick(); });

		traces = std::count_if(score.notes.begin(), score.notes.end(),
		                       [](const auto& n) { return n.second.friction; });

		total = score.notes.size();
		calculateCombo(score);
	}

	void ScoreStats::calculateCombo(const Score& score)
	{
		resetCombo();
		combo = score.notes.size();

		constexpr int halfBeat = TICKS_PER_BEAT / 2;
		for (const auto& [id, hold] : score.holdNotes)
		{
			if (hold.isGuide())
			{
				// Guide holds are not included
				combo -= 2 + hold.steps.size();
				continue;
			}

			// Hidden hold starts and ends do not count towards combo
			if (hold.startType != HoldNoteType::Normal)
				combo--;

			if (hold.endType != HoldNoteType::Normal)
				combo--;

			combo -= std::count_if(hold.steps.begin(), hold.steps.end(),
			                       [](const HoldStep& step)
			                       { return step.type == HoldStepType::Hidden; });

			int startTick = score.notes.at(id).tick;
			int endTick = score.notes.at(hold.end).tick;
			int eighthTick = startTick;

			eighthTick += halfBeat;
			if (eighthTick % halfBeat)
				eighthTick -= (eighthTick % halfBeat);

			// hold <= 1/8th long
			if (eighthTick == startTick || eighthTick == endTick)
				continue;

			if (endTick % halfBeat)
				endTick += halfBeat - (endTick % halfBeat);

			combo += (endTick - eighthTick) / halfBeat;
		}
	}
}
