#include "ScoreStats.h"
#include "Score.h"
#include "Constants.h"

namespace MikuMikuWorld
{
	ScoreStats::ScoreStats()
	{
		reset();
	}

	void ScoreStats::reset()
	{
		resetCounts();
		resetCombo();
	}

	void ScoreStats::resetCounts()
	{
		taps = flicks = holds = steps = total = 0;
	}

	void ScoreStats::resetCombo()
	{
		combo = 0;
	}

	void ScoreStats::calculateStats(const Score& score)
	{
		resetCounts();
		for (const auto& [id, note] : score.notes)
		{
			switch (note.getType())
			{
			case NoteType::Tap:
				note.isFlick() ? ++flicks : ++taps;
				break;

			case NoteType::Hold:
				++holds;
				break;

			case NoteType::HoldMid:
				++steps;
				break;

			case NoteType::HoldEnd:
				if (note.isFlick())
					++flicks;
				break;

			default:
				break;
			}
		}

		total = taps + flicks + holds + steps;
		calculateCombo(score);
	}

	void ScoreStats::calculateCombo(const Score& score)
	{
		resetCombo();
		for (const auto& [id, note] : score.notes)
		{
			if (note.getType() == NoteType::HoldMid)
			{
				const HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, note.ID);
				if (pos != -1)
					if (hold.steps[pos].type == HoldStepType::Hidden)
						continue;
			}

			++combo;
		}

		int halfBeat = TICKS_PER_BEAT / 2;
		for (const auto& hold : score.holdNotes)
		{
			int startTick = score.notes.at(hold.first).tick;
			int endTick = score.notes.at(hold.second.end).tick;
			int eigthTick = startTick;

			eigthTick += halfBeat;
			if (eigthTick % halfBeat)
				eigthTick -= (eigthTick % halfBeat);

			// hold <= 1/8th long
			if (eigthTick == startTick || eigthTick == endTick)
				continue;

			if (endTick % halfBeat)
				endTick += halfBeat - (endTick % halfBeat);

			combo += (endTick - eigthTick) / halfBeat;
		}
	}
}
