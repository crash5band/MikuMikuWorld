#include "Selection.h"

namespace MikuMikuWorld
{
	Selection::Selection() :
		_hasFlick{ false }, _hasEase{ false }, _hasStep{ false }
	{

	}

	bool Selection::_hasSelectionEase(const Score& score)
	{
		for (const int id : selectedNotes)
			if (score.notes.at(id).hasEase())
				return true;

		return false;
	}

	bool Selection::_hasSelectionStep(const Score& score)
	{
		for (const int id : selectedNotes)
			if (score.notes.at(id).getType() == NoteType::HoldMid)
				return true;

		return false;
	}

	bool Selection::_hasSelectionFlick(const Score& score)
	{
		for (const int id : selectedNotes)
			if (!score.notes.at(id).hasEase())
				return true;

		return false;
	}

	void Selection::update(const Score& score)
	{
		_hasEase = _hasSelectionEase(score);
		_hasStep = _hasSelectionStep(score);
		_hasFlick = _hasSelectionFlick(score);
	}

	void Selection::clear()
	{
		selectedNotes.clear();
	}

	void Selection::append(int noteID)
	{
		selectedNotes.insert(noteID);
	}

	void Selection::remove(int noteID)
	{
		selectedNotes.erase(noteID);
	}

	int Selection::count() const
	{
		return selectedNotes.size();
	}

	bool Selection::hasSelection() const
	{
		return count();
	}

	bool Selection::hasEase() const
	{
		return _hasEase;
	}

	bool Selection::hasStep() const
	{
		return _hasStep;
	}

	bool Selection::hasFlickable() const
	{
		return _hasFlick;
	}

	bool Selection::hasNote(int id) const
	{
		return selectedNotes.find(id) != selectedNotes.end();
	}

	const std::unordered_set<int>& Selection::getSelection()
	{
		return selectedNotes;
	}

	std::unordered_set<int> Selection::getHolds(const Score& score)
	{
		std::unordered_set<int> holds;
		for (auto& id : selectedNotes)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
				holds.insert(note.ID);
			else if (note.getType() == NoteType::HoldMid || note.getType() == NoteType::HoldEnd)
				holds.insert(note.parentID);
		}

		return holds;
	}
}