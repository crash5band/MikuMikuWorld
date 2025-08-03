#include "NotesFilter.h"

#include <algorithm>
#include <iterator>

namespace MikuMikuWorld
{
	bool noteParentExists(const Note& note, const Score& score)
	{
		return score.holdNotes.find(note.parentID) != score.holdNotes.end();
	}
	
	bool FlickableNotesFilter::canFlick(int noteId, const Score& score) const
	{
		const auto it = score.notes.find(noteId);
		if (it == score.notes.end())
			return false;

		const Note& note = it->second;
		if (note.getType() == NoteType::HoldEnd && noteParentExists(note, score))
			return score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
		
		return !note.hasEase();
	}

	NoteSelection FlickableNotesFilter::filter(NoteSelection selection, const Score& score) const
	{
		NoteSelection newSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(newSelection, newSelection.begin()),
			[&](const int id) { return canFlick(id, score); });

		return newSelection;
	}
	
	NoteSelection HoldStepNotesFilter::filter(NoteSelection selection, const Score& score) const
	{
		NoteSelection newSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(newSelection, newSelection.begin()),
			[&](const int id)
			{
				auto it = score.notes.find(id);
				if (it == score.notes.end())
					return false;

				return  it->second.getType() == NoteType::HoldMid && noteParentExists(it->second, score);
			}
		);

		return newSelection;
	}

	bool FrictionableNotesFilter::canToggleFriction(int noteId, const Score& score) const
	{
		const auto it = score.notes.find(noteId);
		if (it == score.notes.end())
			return false;

		const Note& note = it->second;
		switch (note.getType())
		{
		case NoteType::HoldMid:
			return false;
		case NoteType::Hold:
			return !score.holdNotes.at(note.ID).isGuide();
		case NoteType::HoldEnd:
			return !score.holdNotes.at(note.parentID).isGuide();
		default:
			return true;
		}
	}
	
	NoteSelection FrictionableNotesFilter::filter(NoteSelection selection, const Score& score) const
	{
		NoteSelection newSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(newSelection, newSelection.begin()),
			[&](const int id) { return canToggleFriction(id, score); });

		return newSelection;
	}

	NoteSelection InverseNotesFilter::filter(NoteSelection selection, const Score& score) const
	{
		if (originalFilter == nullptr)
			return selection;

		NoteSelection filteredSelection = originalFilter->filter(selection, score);
		NoteSelection inverseSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(inverseSelection, inverseSelection.begin()),
			[&filteredSelection](const int i) { return filteredSelection.find(i) == filteredSelection.end(); });

		return inverseSelection;
	}

	bool GuideNotesFilter::isGuideHold(int noteId, const Score& score) const
	{
		const auto it = score.notes.find(noteId);
		if (it == score.notes.end())
			return false;

		const Note& note = it->second;
		switch (note.getType())
		{
		case NoteType::Hold:
			return score.holdNotes.at(note.ID).isGuide();
		case NoteType::HoldMid:
		case NoteType::HoldEnd:
			return noteParentExists(note, score) && score.holdNotes.at(note.parentID).isGuide();
		default:
			return false;
		}
	}
	
	NoteSelection GuideNotesFilter::filter(NoteSelection selection, const Score& score) const
	{
		NoteSelection newSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(newSelection, newSelection.begin()),
			[&](const int id) { return isGuideHold(id, score); });

		return newSelection;
	}

	NoteSelection EaseNotesFilter::filter(NoteSelection selection, const Score& score) const
	{
		NoteSelection newSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(newSelection, newSelection.begin()),
			[&](const int id) { return score.notes.find(id) != score.notes.end() && score.notes.at(id).hasEase(); });

		return newSelection;
	}

	NoteSelection CustomFilter::filter(NoteSelection selection, const Score& score) const
	{
		NoteSelection newSelection;
		std::copy_if(selection.begin(), selection.end(), std::inserter(newSelection, newSelection.begin()), predicate);

		return newSelection;
	}

}
