#include "Note.h"
#include "Constants.h"
#include "Score.h"
#include <algorithm>

namespace MikuMikuWorld
{
	int nextID = 1;

	Note::Note(NoteType _type) :
		type{ _type }, parentID{ -1 }, flick{ FlickType::None }, critical{ false }
	{

	}

	Note::Note() : type{ NoteType::Tap }, parentID{ -1 }, flick{ FlickType::None }, critical{ false }
	{

	}

	NoteType Note::getType() const
	{
		return type;
	}

	bool Note::isFlick() const
	{
		return flick != FlickType::None && type != NoteType::Hold && type != NoteType::HoldMid;
	}

	bool Note::hasEase() const
	{
		return type == NoteType::Hold || type == NoteType::HoldMid;
	}

	void Note::setLane(int l)
	{
		lane = std::clamp(l, MIN_LANE, MAX_LANE - width + 1);
	}

	void Note::setWidth(int w)
	{
		width = std::clamp(w, MIN_NOTE_WIDTH, MAX_NOTE_WIDTH - lane);
	}

	void resetNextID()
	{
		nextID = 1;
	}

	void cycleFlick(Note& note)
	{
		if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldMid)
			return;

		note.flick = (FlickType)(((int)note.flick + 1) % 4);
	}

	void cycleStepEase(HoldStep& note)
	{
		EaseType ease = (EaseType)(((int)note.ease + 1) % 3);
		setStepEase(note, ease);
	}

	void setStepEase(HoldStep& step, EaseType ease)
	{
		step.ease = ease;
	}

	void cycleStepType(HoldStep& note)
	{
		note.type = (HoldStepType)(((int)note.type + 1) % 2);
	}

	void sortHoldSteps(const Score& score, HoldNote& note)
	{
		using tickStepPair = std::pair<int, HoldStep>;

		std::vector<tickStepPair> stepsMap;
		stepsMap.reserve(note.steps.size());
		for (auto& step : note.steps)
			stepsMap.push_back(tickStepPair(score.notes.at(step.ID).tick, step));

		std::stable_sort(stepsMap.begin(), stepsMap.end(), [](const tickStepPair& a, const tickStepPair& b) {
			return a.first < b.first;
		});

		note.steps.clear();
		for (const auto& it : stepsMap)
			note.steps.push_back(it.second);
	}

	int getFlickArrowSpriteIndex(const Note& note)
	{
		int startIndex = note.critical ? 16 : 4;
		return startIndex + ((std::min(note.width, 6) - 1) * 2) + (note.flick != FlickType::Up ? 1 : 0);
	}

	int getNoteSpriteIndex(const Note& note)
	{
		// default tap
		int index = 3;

		if (note.critical && note.getType() != NoteType::HoldMid)
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
				index = note.critical ? 29 : 28;
				break;

			default:
				break;
			}
		}

		return index;
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

	std::string getNoteSE(const Note& note, const Score& score)
	{
		std::string se = "";
		if (note.getType() == NoteType::HoldMid)
		{
			const HoldNote& hold = score.holdNotes.at(note.parentID);
			int pos = findHoldStep(hold, note.ID);
			if (pos != -1 && hold.steps[pos].type == HoldStepType::Invisible)
				return "";

			se = note.critical ? SE_CRITICAL_TICK : SE_TICK;
		}
		else
		{
			se = note.isFlick() ? note.critical ? SE_CRITICAL_FLICK : SE_FLICK :
				note.critical ? note.getType() == NoteType::Tap ? SE_CRITICAL_TAP : SE_PERFECT : SE_PERFECT;
		}

		return se;
	}
}
