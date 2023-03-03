#include "ScoreEditor.h"
#include "Score.h"
#include "Colors.h"
#include "HistoryManager.h"
#include "Clipboard.h"
#include <map>
#include <math.h>

namespace MikuMikuWorld
{
	void ScoreEditor::selectAll()
	{
		for (const auto& note : score.notes)
			selection.append(note.first);
	}

	void ScoreEditor::clearSelection()
	{
		selection.clear();
	}

	bool ScoreEditor::isAnyNoteSelected() const
	{
		return selection.count();
	}

	void ScoreEditor::copy()
	{
		if (!selection.hasSelection())
			return;

		Clipboard::copy(score, selection.getSelection());
	}

	void ScoreEditor::cut()
	{
		copy();
		deleteSelected();
	}

	void ScoreEditor::paste()
	{
		if (pasting)
			return;

		if (Clipboard::hasData())
		{
			pasteLane = canvas.positionToLane(mousePos.x);
			pasting = true;
		}
	}

	void ScoreEditor::flipPaste()
	{
		if (flipPasting)
			return;

		if (Clipboard::hasData())
		{
			pasteLane = canvas.positionToLane(mousePos.x);
			flipPasting = true;
		}
	}

	void ScoreEditor::cancelPaste()
	{
		pasting = flipPasting = insertingPreset = false;
	}

	void ScoreEditor::confirmPaste()
	{
		const std::unordered_map<int, Note>& pasteNotes = isPasting() ?
			Clipboard::getData(flipPasting).notes : presetManager.getSelected().notes;

		const std::unordered_map<int, HoldNote>& pasteHolds = isPasting() ?
			Clipboard::getData(flipPasting).holds : presetManager.getSelected().holds;

		Score prev = score;
		selection.clear();

		score.notes.reserve(score.notes.size() + pasteNotes.size());
		for (auto[id, note] : pasteNotes)
		{
			if (note.getType() == NoteType::Tap)
			{
				int newID = nextID++;
				note.ID = newID;
				note.tick += hoverTick;
				note.setLane(note.lane + canvas.positionToLane(mousePos.x) - pasteLane);
				score.notes[newID] = note;
				selection.append(note.ID);
			}
		}

		score.holdNotes.reserve(score.holdNotes.size() + pasteHolds.size());
		for (auto[id, hold] : pasteHolds)
		{
			HoldNote hold = pasteHolds.at(id);
			int startID = nextID++;
			int endID = nextID++;

			Note start = pasteNotes.at(hold.start.ID);
			start.ID = startID;
			start.tick += hoverTick;
			start.setLane(start.lane + canvas.positionToLane(mousePos.x) - pasteLane);
			hold.start.ID = startID;
			selection.append(hold.start.ID);

			Note end = pasteNotes.at(hold.end);
			end.ID = endID;
			end.parentID = startID;
			end.tick += hoverTick;
			end.setLane(end.lane + canvas.positionToLane(mousePos.x) - pasteLane);
			hold.end = endID;
			selection.append(hold.end);

			score.notes[startID] = start;
			score.notes[endID] = end;

			// by reference here because we are modifying the steps of the newly created hold
			for (auto &step : hold.steps)
			{
				Note mid = pasteNotes.at(step.ID);
				mid.ID = nextID++;
				mid.parentID = startID;
				mid.tick += hoverTick;
				mid.setLane(mid.lane + canvas.positionToLane(mousePos.x) - pasteLane);
				step.ID = mid.ID;
				selection.append(step.ID);

				score.notes[mid.ID] = mid;
			}

			score.holdNotes[startID] = hold;
		}

		pasting = flipPasting = insertingPreset = false;

		pushHistory("Paste notes", prev, score);
	}

	bool ScoreEditor::isPasting() const
	{
		return pasting || flipPasting;
	}

	void ScoreEditor::previewPaste(Renderer* renderer)
	{
		const std::unordered_map<int, Note>& pasteNotes = isPasting() ?
			Clipboard::getData(flipPasting).notes : presetManager.getSelected().notes;

		const std::unordered_map<int, HoldNote>& pasteHolds = isPasting() ?
			Clipboard::getData(flipPasting).holds : presetManager.getSelected().holds;

		int lane = canvas.positionToLane(mousePos.x) - pasteLane;
		for (const auto& [id, note] : pasteNotes)
		{
			if (note.getType() == NoteType::Tap)
			{
				if (canvas.isNoteInCanvas(note.tick + hoverTick))
					drawNote(note, renderer, hoverTint, hoverTick, lane);
			}
		}

		for (const auto& copy : pasteHolds)
			drawHoldNote(pasteNotes, copy.second, renderer, hoverTint, hoverTick, lane);
	}

	void ScoreEditor::deleteSelected()
	{
		if (!selection.hasSelection())
			return;

		Score prev = score;

		for (auto& id : selection.getSelection())
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

		selection.clear();
		pushHistory("Delete notes", prev, score);
	}

	void ScoreEditor::shrinkSelected(int direction)
	{
		if (selection.count() < 2)
			return;

		Score prev = score;
		std::vector<int> sortedSelection(selection.getSelection().size());
		std::copy(selection.getSelection().begin(), selection.getSelection().end(), sortedSelection.begin());
		std::sort(sortedSelection.begin(), sortedSelection.end(), [prev](int a, int b) {
			return prev.notes.at(a).tick < prev.notes.at(b).tick;
		});

		if(direction == 0){
			int i = 0;
			int firstTick = score.notes.at(*sortedSelection.begin()).tick;
			
			for (auto& id : sortedSelection)
			{
				auto& note = score.notes.at(id);
				note.tick = firstTick + (i++);
				if (note.parentID != -1)
					sortHoldSteps(score, score.holdNotes.at(note.parentID));
			}
		}else if(direction == 1) {
			int i = -selection.count() + 1;
			int lastTick = score.notes.at(sortedSelection.back()).tick;
			for (auto& id : sortedSelection)
			{
				auto& note = score.notes.at(id);
				note.tick = lastTick + (i++);
				if (note.parentID != -1)
					sortHoldSteps(score, score.holdNotes.at(note.parentID));
			}
		}

		pushHistory("Shrink notes", prev, score);
	}

	void ScoreEditor::insertNote(bool critical)
	{
		Score prev = score;

		dummy.ID = nextID++;
		score.notes[dummy.ID] = dummy;

		pushHistory("Insert note", prev, score);
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
		pushHistory("Insert note", prev, score);

		audio.playSound("perfect", audio.getEngineAbsTime(), -1);
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

		pushHistory("Insert note", prev, score);
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
		note.steps.push_back(HoldStep{ mid.ID, defaultStepType, EaseType::None });
		sortHoldSteps(score, note);

		pushHistory("Insert note", prev, score);
	}

	void ScoreEditor::cycleFlicks()
	{
		if (!selection.hasSelection())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selection.getSelection())
		{
			Note& note = score.notes.at(id);
			if (!note.hasEase())
			{
				cycleFlick(note);
				if (note.getType() == NoteType::HoldEnd && !note.isFlick() && note.critical)
					note.critical = score.notes.at(note.parentID).critical;

				edit = true;
			}
		}

		if (edit)
			pushHistory("Change note", prev, score);
	}

	void ScoreEditor::setFlick(FlickType flick)
	{
		if (!selection.hasSelection())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selection.getSelection())
		{
			Note& note = score.notes.at(id);
			if (!note.hasEase())
			{
				note.flick = flick;
				edit = true;
			}
		}

		if (edit)
			pushHistory("Change flick", prev, score);
	}

	void ScoreEditor::cycleEase()
	{
		if (!selection.hasSelection())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selection.getSelection())
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				cycleStepEase(score.holdNotes.at(note.ID).start);
				edit = true;
			}
			else if (note.getType() == NoteType::HoldMid)
			{
				HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, id);
				if (pos != -1)
				{
					cycleStepEase(hold.steps[pos]);
					edit = true;
				}
			}
		}

		if (edit)
			pushHistory("Change note", prev, score);
	}

	void ScoreEditor::setEase(EaseType ease)
	{
		if (!selection.hasSelection())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selection.getSelection())
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				score.holdNotes.at(note.ID).start.ease = ease;
				edit = true;
			}
			else if (note.getType() == NoteType::HoldMid)
			{
				HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, id);
				if (pos != -1)
				{
					hold.steps[pos].ease = ease;
					edit = true;
				}
			}
		}

		if (edit)
			pushHistory("Change note", prev, score);
	}

	void ScoreEditor::toggleCriticals()
	{
		if (!selection.hasSelection())
			return;

		Score prev = score;
		std::unordered_set<int> critHolds;
		for (int id : selection.getSelection())
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

		pushHistory("Change note", prev, score);
	}

	void ScoreEditor::setStepType(HoldStepType type)
	{
		if (!selection.hasSelection())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selection.getSelection())
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::HoldMid)
				continue;

			HoldNote& hold = score.holdNotes.at(note.parentID);
			int pos = findHoldStep(hold, id);
			if (pos != -1)
			{
				hold.steps[pos].type = type;
				edit = true;
			}
		}

		if (edit)
			pushHistory("Change note", prev, score);
	}

	void ScoreEditor::cycleStepType()
	{
		if (!selection.hasSelection())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selection.getSelection())
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::HoldMid)
				continue;

			HoldNote& hold = score.holdNotes.at(note.parentID);
			int pos = findHoldStep(hold, id);
			if (pos != -1)
			{
				hold.steps[pos].type = (HoldStepType)(((int)hold.steps[pos].type + 1) % 3);
				edit = true;
			}
		}

		if (edit)
			pushHistory("Change note", prev, score);
	}

	void ScoreEditor::flipSelected()
	{
		if (!selection.hasSelection())
			return;

		Score prev = score;
		flip();

		pushHistory("Flip note(s)", prev, score);
	}

	void ScoreEditor::flip()
	{
		for (int id : selection.getSelection())
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
		score.tempoChanges.push_back(Tempo{ tick, defaultBPM });
		std::sort(score.tempoChanges.begin(), score.tempoChanges.end(),
			[](Tempo& t1, Tempo& t2) { return t1.tick < t2.tick; });

		pushHistory("Add tempo change", prev, score);
	}

	void ScoreEditor::insertTimeSignature()
	{
		int measure = accumulateMeasures(hoverTick, TICKS_PER_BEAT, score.timeSignatures);
		if (score.timeSignatures.find(measure) != score.timeSignatures.end())
			return;

		Score prev = score;
		score.timeSignatures[measure] = TimeSignature{ measure, defaultTimeSignN, defaultTimeSignD };
		pushHistory("Add time signature change", prev, score);
	}

	void ScoreEditor::insertHiSpeedChange()
	{
		int tick = hoverTick;
		for (const auto& hiSpeed : score.hiSpeedChanges)
			if (hiSpeed.tick == tick)
				return;

		Score prev = score;
		score.hiSpeedChanges.push_back({ tick, defaultHiSpeed });
		std::sort(score.hiSpeedChanges.begin(), score.hiSpeedChanges.end(),
			[](HiSpeedChange& a, HiSpeedChange& b) { return a.tick < b.tick; });

		pushHistory("Add hi-speed change", prev, score);
	}
}
