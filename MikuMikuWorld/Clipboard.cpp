#include "Clipboard.h"
#include "Constants.h"

namespace MikuMikuWorld
{
	ClipboardNotes Clipboard::data;
	ClipboardNotes Clipboard::flippedData;

	void Clipboard::copy(const Score& score, const std::unordered_set<int>& selection)
	{
		data.notes.clear();
		data.holds.clear();

		int leastTick = score.notes.at(*selection.begin()).tick;

		std::unordered_set<int> holds;
		for (int id : selection)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::Tap)
			{
				holds.insert(note.getType() == NoteType::Hold ? note.ID : note.parentID);
			}
			else
			{
				data.notes[note.ID] = note;
			}

			if (note.tick < leastTick)
				leastTick = note.tick;
		}

		data.holds.reserve(holds.size());
		for (int id : holds)
		{
			const HoldNote& hold = score.holdNotes.at(id);
			data.notes[hold.start.ID] = score.notes.at(hold.start.ID);
			data.notes[hold.end] = score.notes.at(hold.end);

			for (const auto& step : hold.steps)
				data.notes[step.ID] = score.notes.at(step.ID);

			data.holds[hold.start.ID] = hold;
		}

		// offset ticks
		for (auto& note : data.notes)
			note.second.tick -= leastTick;

		flippedData.notes = data.notes;
		flippedData.holds = data.holds;

		// flip copyNotesFlip
		for (auto& it : flippedData.notes)
		{
			Note& note = it.second;
			note.lane = MAX_LANE - note.lane - note.width + 1;

			if (note.flick == FlickType::Left)
				note.flick = FlickType::Right;
			else if (note.flick == FlickType::Right)
				note.flick = FlickType::Left;
		}
	}

	const ClipboardNotes& Clipboard::getData(bool flipped)
	{
		return flipped ? flippedData : data;
	}

	int Clipboard::count()
	{
		return data.notes.size();
	}

	bool Clipboard::hasData()
	{
		return count();
	}
}