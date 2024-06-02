#include "Note.h"
#include "Constants.h"
#include "Score.h"
#include <algorithm>

namespace MikuMikuWorld
{
	int nextID = 1;
	int nextSkillID = 1;

	Note::Note(NoteType _type) :
		type{ _type }, parentID{ -1 }, tick{ 0 }, lane{ 0 }, width{ 3 }, critical { false }, friction{ false }
	{

	}

	Note::Note(NoteType _type, int _tick, int _lane, int _width) :
		type{ _type }, parentID{ -1 }, tick{ _tick }, lane{ _lane }, width{ _width }, critical{ false }, friction{ false }
	{

	}

	Note::Note() :
		type{ NoteType::Tap }, parentID{ -1 }, tick{ 0 }, lane{ 0 }, width{ 3 }, critical{ false }, friction{ false }
	{

	}

	bool Note::isFlick() const
	{
		return flick != FlickType::None && type != NoteType::Hold && type != NoteType::HoldMid;
	}

	bool Note::hasEase() const
	{
		return type == NoteType::Hold || type == NoteType::HoldMid;
	}

	void resetNextID()
	{
		nextID = 1;
	}

	void cycleFlick(Note& note)
	{
		if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldMid)
			return;

		note.flick = (FlickType)(((int)note.flick + 1) % (int)FlickType::FlickTypeCount);
	}

	void cycleStepEase(HoldStep& note)
	{
		note.ease = (EaseType)(((int)note.ease + 1) % (int)EaseType::EaseTypeCount);
	}

	void cycleStepType(HoldStep& note)
	{
		note.type = (HoldStepType)(((int)note.type + 1) % (int)HoldStepType::HoldStepTypeCount);
	}

	void swapNotesPosition(Note& n1, Note& n2)
	{
		std::swap(n1.tick, n2.tick);
		std::swap(n1.lane, n2.lane);
		n1.lane = std::clamp(n1.lane, MIN_LANE, MAX_LANE - n1.width + 1);
		n2.lane = std::clamp(n2.lane, MIN_LANE, MAX_LANE - n2.width + 1);
	}

	void sortHold(Score& score, HoldNote& hold)
	{
		Note& start = score.notes.at(hold.start.ID);
		Note& end = score.notes.at(hold.end);

		if (start.tick > end.tick)
		{
			std::swap(start.tick, end.tick);
			std::swap(start.lane, end.lane);
			std::swap(start.width, end.width);
		}
		
		if (!hold.steps.empty())
		{
			sortHoldSteps(score, hold);
			
			Note& firstMid = score.notes.at(hold.steps[0].ID);
			if (start.tick > firstMid.tick)
				swapNotesPosition(start, firstMid);

			Note& lastMid = score.notes.at(hold.steps[hold.steps.size() - 1].ID);
			if (end.tick < lastMid.tick)
				swapNotesPosition(end, lastMid);

			sortHoldSteps(score, hold);
		}
	}

	void sortHoldSteps(const Score& score, HoldNote& note)
	{
		std::stable_sort(note.steps.begin(), note.steps.end(),
			[&score](const HoldStep& s1, const HoldStep& s2)
			{
				const Note& n1 = score.notes.at(s1.ID);
				const Note& n2 = score.notes.at(s2.ID);
				return n1.tick == n2.tick ? n1.lane < n2.lane : n1.tick < n2.tick;
			}
		);
	}

	int getFlickArrowSpriteIndex(const Note& note)
	{
		int startIndex = note.critical ? 24 : 12;
		return startIndex + ((std::min(note.width, 6) - 1) * 2) + (note.flick != FlickType::Default ? 1 : 0);
	}

	int getNoteSpriteIndex(const Note& note)
	{
		// default tap
		int index = 3;

		if (note.friction)
		{
			index = note.critical ? 5 : note.flick != FlickType::None ? 6 : 4;
		}
		else if (note.critical && note.getType() != NoteType::HoldMid)
		{
			index = 0;
		}
		else if (note.isFlick())
		{
			index = 1;
		}
		else
		{
			switch (note.getType())
			{
			case NoteType::Hold:
			case NoteType::HoldEnd:
				index = 2;
				break;

			case NoteType::HoldMid:
				index = note.critical ? 8 : 7;
				break;

			default:
				break;
			}
		}

		return index;
	}

	int getFrictionSpriteIndex(const Note& note)
	{
		return note.critical ? 10 : note.flick != FlickType::None ? 11 : 9;
	}

	int findHoldStep(const HoldNote& note, int stepID)
	{
		for (int index = 0; index < note.steps.size(); ++index)
		{
			if (note.steps[index].ID == stepID)
				return index;
		}

		return -1;
	}

	std::string_view getNoteSE(const Note& note, const Score& score)
	{
		std::string_view se = SE_PERFECT;
		if (note.getType() == NoteType::HoldMid)
		{
			const HoldNote& hold = score.holdNotes.at(note.parentID);
			int pos = findHoldStep(hold, note.ID);
			if (pos != -1 && hold.steps[pos].type == HoldStepType::Hidden)
				return "";

			se = note.critical ? SE_CRITICAL_TICK : SE_TICK;
		}
		else
		{
			if (note.isFlick())
			{
				se = note.critical ? SE_CRITICAL_FLICK : SE_FLICK;
			}
			else if (note.friction)
			{
				se = note.critical ? SE_CRITICAL_FRICTION : SE_FRICTION;
			}
			else if (note.critical && note.getType() == NoteType::Tap)
			{
				se = SE_CRITICAL_TAP;
			}
		}

		return se;
	}
}
