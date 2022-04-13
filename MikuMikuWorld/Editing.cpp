#include "ScoreEditor.h"
#include "Score.h"
#include "Colors.h"
#include "HistoryManager.h"
#include <map>
#include <math.h>

namespace MikuMikuWorld
{
	void ScoreEditor::selectAll()
	{
		for (const auto& note : score.notes)
			selectedNotes.insert(note.first);
	}

	void ScoreEditor::clearSelection()
	{
		selectedNotes.clear();
	}

	void ScoreEditor::copy()
	{
		int leastTick = 0;
		if (selectedNotes.size())
		{
			copyNotes.clear();
			copyHolds.clear();
			leastTick = score.notes.at(*selectedNotes.begin()).tick;
		}

		std::unordered_set<int> holds;
		for (int id : selectedNotes)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::Tap)
			{
				holds.insert(note.getType() == NoteType::Hold ? note.ID : note.parentID);
			}
			else
			{
				copyNotes[note.ID] = note;
			}

			if (note.tick < leastTick)
				leastTick = note.tick;
		}

		for (int id : holds)
		{
			const HoldNote& hold = score.holdNotes.at(id);
			copyNotes[hold.start.ID] = score.notes.at(hold.start.ID);
			copyNotes[hold.end] = score.notes.at(hold.end);

			for (const auto& step : hold.steps)
				copyNotes[step.ID] = score.notes.at(step.ID);

			copyHolds[hold.start.ID] = hold;
		}

		// offset ticks
		for (auto& note : copyNotes)
			note.second.tick -= leastTick;

		copyNotesFlip = copyNotes;

		// flip copyNotesFlip
		for (auto& it : copyNotesFlip)
		{
			Note& note = it.second;
			note.lane = MAX_LANE - note.lane - note.width + 1;

			if (note.flick == FlickType::Left)
				note.flick = FlickType::Right;
			else if (note.flick == FlickType::Right)
				note.flick = FlickType::Left;
		}
	}

	void ScoreEditor::paste()
	{
		if (pasting)
			return;

		if (hasClipboard())
		{
			pasteLane = positionToLane(mousePos.x);
			pasting = true;
		}
	}

	void ScoreEditor::flipPaste()
	{
		if (flipPasting)
			return;

		if (hasClipboard())
		{
			pasteLane = positionToLane(mousePos.x);
			flipPasting = true;
		}
	}

	void ScoreEditor::cancelPaste()
	{
		pasting = flipPasting = false;
	}

	void ScoreEditor::confirmPaste()
	{
		Score prev = score;

		std::unordered_map<int, Note>& pasteNotes = flipPasting ? copyNotesFlip : copyNotes;
		selectedNotes.clear();

		for (auto& copy : pasteNotes)
		{
			Note note = copy.second;
			if (note.getType() == NoteType::Tap)
			{
				int newID = nextID++;
				note.ID = newID;
				note.tick += hoverTick;
				note.setLane(note.lane + positionToLane(mousePos.x) - pasteLane);
				score.notes[newID] = note;
				selectedNotes.insert(note.ID);
			}
		}

		for (auto& copy : copyHolds)
		{
			HoldNote hold = copyHolds[copy.first];
			int startID = nextID++;
			int endID = nextID++;

			Note start = pasteNotes[hold.start.ID];
			start.ID = startID;
			start.tick += hoverTick;
			start.setLane(start.lane + positionToLane(mousePos.x) - pasteLane);
			hold.start.ID = startID;
			selectedNotes.insert(hold.start.ID);

			Note end = pasteNotes[hold.end];
			end.ID = endID;
			end.parentID = startID;
			end.tick += hoverTick;
			end.setLane(end.lane + positionToLane(mousePos.x) - pasteLane);
			hold.end = endID;
			selectedNotes.insert(hold.end);

			score.notes[startID] = start;
			score.notes[endID] = end;

			for (auto& step : hold.steps)
			{
				Note mid = pasteNotes[step.ID];
				mid.ID = nextID++;
				mid.parentID = startID;
				mid.tick += hoverTick;
				mid.setLane(mid.lane + positionToLane(mousePos.x) - pasteLane);
				step.ID = mid.ID;
				selectedNotes.insert(step.ID);

				score.notes[mid.ID] = mid;
			}

			score.holdNotes[startID] = hold;
		}

		pasting = flipPasting = false;

		history.pushHistory("Paste notes", prev, score);
	}

	bool ScoreEditor::hasClipboard() const
	{
		return copyNotes.size();
	}

	bool ScoreEditor::isPasting() const
	{
		return pasting || flipPasting;
	}

	void ScoreEditor::previewPaste(Renderer* renderer)
	{
		std::unordered_map<int, Note>& pasteNotes = flipPasting ? copyNotesFlip : copyNotes;

		int lane = positionToLane(mousePos.x) - pasteLane;
		for (const auto& copy : pasteNotes)
		{
			int id = copy.first;
			const Note& note = copy.second;
			if (note.getType() == NoteType::Tap)
			{
				if (isNoteInCanvas(note.tick + hoverTick))
					drawNote(note, renderer, hoverTint, hoverTick, lane);
			}
		}

		for (const auto& copy : copyHolds)
			drawHoldNote(pasteNotes, copy.second, renderer, hoverTint, hoverTick, lane);
	}

	void ScoreEditor::deleteSelected()
	{
		Score prev = score;

		for (auto& id : selectedNotes)
		{
			auto notePos = score.notes.find(id);
			if (notePos == score.notes.end())
				continue;

			Note& note = notePos->second;
			if (note.getType() != NoteType::Hold && note.getType() != NoteType::HoldEnd)
			{
				if (note.getType() == NoteType::HoldMid)
				{
					if (score.holdNotes.find(note.parentID) != score.holdNotes.end())
					{
						for (auto it = score.holdNotes.at(note.parentID).steps.begin(); it != score.holdNotes.at(note.parentID).steps.end(); ++it)
							if (it->ID == id) { score.holdNotes.at(note.parentID).steps.erase(it); break; }
					}
				}
				score.notes.erase(id);
			}
			else
			{
				const HoldNote& hold = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);
				score.notes.erase(hold.start.ID);
				score.notes.erase(hold.end);

				for (const auto& step : hold.steps)
					score.notes.erase(step.ID);

				score.holdNotes.erase(hold.start.ID);
			}
		}

		selectedNotes.clear();

		history.pushHistory("Delete notes", prev, score);
	}

	void ScoreEditor::insertNote(bool critical)
	{
		Score prev = score;

		dummy.ID = nextID++;
		score.notes[dummy.ID] = dummy;

		history.pushHistory("Insert note", prev, score);
	}

	void ScoreEditor::insertNotePlaying()
	{
		Score prev = score;

		Note note{ NoteType::Tap };
		note.ID = nextID++;
		note.critical = false;
		note.lane = 0;
		note.width = defaultNoteWidth;
		note.tick = snapTick(currentTick, division);

		score.notes[note.ID] = note;
		history.pushHistory("Insert note", prev, score);

		audio.pushAudioEvent("perfect", audio.getEngineAbsTime(), -1, false);
	}

	void ScoreEditor::insertHoldNote()
	{
		Score prev = score;

		if (dummyStart.tick == dummyEnd.tick)
			dummyEnd.tick += (TICKS_PER_BEAT / (division / 4));
		else if (dummyEnd.tick < dummyStart.tick)
		{
			std::swap(dummyStart.lane, dummyEnd.lane);
			std::swap(dummyStart.tick, dummyEnd.tick);
		}

		dummyStart.ID = nextID++;
		dummyEnd.ID = nextID++;
		dummyEnd.parentID = dummyStart.ID;
		score.notes[dummyStart.ID] = dummyStart;
		score.notes[dummyEnd.ID] = dummyEnd;

		HoldNote hold;
		HoldStep start{ dummyStart.ID, HoldStepType::Visible, EaseType::None };
		hold.start = start;
		hold.end = dummyEnd.ID;
		score.holdNotes[dummyStart.ID] = hold;

		history.pushHistory("Insert note", prev, score);
	}

	void ScoreEditor::insertHoldStep(HoldNote& note)
	{
		Score prev = score;

		Note mid(NoteType::HoldMid);
		mid.width = defaultNoteWidth;
		mid.lane = laneFromCenterPos(hoverLane, mid.width);
		mid.tick = hoverTick;
		mid.critical = score.notes.at(note.start.ID).critical;
		mid.ID = nextID++;
		mid.parentID = note.start.ID;

		score.notes[mid.ID] = mid;
		note.steps.push_back(HoldStep{ mid.ID, HoldStepType::Visible, EaseType::None });
		sortHoldSteps(score, note);

		history.pushHistory("Insert note", prev, score);
	}

	void ScoreEditor::cycleFlicks()
	{
		Score prev = score;

		for (int id : selectedNotes)
			cycleFlick(score.notes.at(id));

		history.pushHistory("Change note", prev, score);
	}

	void ScoreEditor::cycleEase()
	{
		Score prev = score;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				cycleStepEase(score.holdNotes.at(note.ID).start);
			}
			else if (note.getType() == NoteType::HoldMid)
			{
				HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, id);
				if (pos != -1)
					cycleStepEase(hold.steps[pos]);
			}
		}

		history.pushHistory("Change note", prev, score);
	}

	void ScoreEditor::setEase(EaseType ease)
	{
		Score prev = score;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				score.holdNotes.at(note.ID).start.ease = ease;
			}
			else if (note.getType() == NoteType::HoldMid)
			{
				HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, id);
				if (pos != -1)
					hold.steps[pos].ease = ease;
			}
		}

		history.pushHistory("Change note", prev, score);
	}

	void ScoreEditor::toggleCriticals()
	{
		Score prev = score;
		std::unordered_set<int> critHolds;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Tap)
			{
				note.critical ^= true;
			}
			else if (note.getType() == NoteType::HoldEnd && note.isFlick())
			{
				// if the start is critical the entire hold must be critical
				note.critical = score.notes.at(note.parentID).critical ? true : !note.critical;
			}
			else
			{
				critHolds.insert(note.getType() == NoteType::Hold ? note.ID : note.parentID);
			}
		}

		for (auto& hold : critHolds)
		{
			HoldNote& note = score.holdNotes.at(hold);
			bool critical = !score.notes.at(note.start.ID).critical;

			score.notes.at(note.start.ID).critical = critical;
			score.notes.at(note.end).critical = critical;
			for (auto& step : note.steps)
				score.notes.at(step.ID).critical = critical;
		}

		history.pushHistory("Change note", prev, score);
	}

	void ScoreEditor::setStepVisibility(HoldStepType type)
	{
		Score prev = score;
		for (int id : selectedNotes)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::HoldMid)
				continue;

			HoldNote& hold = score.holdNotes.at(note.parentID);
			int pos = findHoldStep(hold, id);
			if (pos != -1)
				hold.steps[pos].type = type;
		}

		history.pushHistory("Change note", prev, score);
	}

	void ScoreEditor::toggleStepVisibility()
	{
		Score prev = score;
		for (int id : selectedNotes)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::HoldMid)
				continue;

			HoldNote& hold = score.holdNotes.at(note.parentID);
			int pos = findHoldStep(hold, id);
			if (pos != -1)
				hold.steps[pos].type = (HoldStepType)(((int)hold.steps[pos].type + 1) % 2);
		}

		history.pushHistory("Change note", prev, score);
	}

	void ScoreEditor::flipSelected()
	{
		if (!selectedNotes.size())
			return;

		Score prev = score;
		flip();

		history.pushHistory("Flip note(s)", prev, score);
	}

	void ScoreEditor::flip()
	{
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			note.lane = MAX_LANE - note.lane - note.width + 1;

			if (note.flick == FlickType::Left)
				note.flick = FlickType::Right;
			else if (note.flick == FlickType::Right)
				note.flick = FlickType::Left;
		}
	}

	void ScoreEditor::insertTempo()
	{
		int tick = hoverTick;
		for (const auto& tempo : score.tempoChanges)
			if (tempo.tick == tick)
				return;

		Score prev = score;
		score.tempoChanges.push_back(Tempo{ tick, 120 });
		std::sort(score.tempoChanges.begin(), score.tempoChanges.end(),
			[](Tempo& t1, Tempo& t2) { return t1.tick < t2.tick; });

		history.pushHistory("Add tempo change", prev, score);
	}

	void ScoreEditor::insertTimeSignature()
	{
		int measure = accumulateMeasures(hoverTick, TICKS_PER_BEAT, score.timeSignatures);
		if (score.timeSignatures.find(measure) != score.timeSignatures.end())
			return;

		Score prev = score;
		score.timeSignatures[measure] = TimeSignature{ measure, 4, 4 };
		history.pushHistory("Add time signature change", prev, score);
	}
}