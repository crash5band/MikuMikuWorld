#include "ScoreContext.h"
#include "Constants.h"
#include "IO.h"
#include "Utilities.h"
#include "UI.h"
#include <stdio.h>
#include <vector>

using json = nlohmann::json;
using namespace IO;

namespace MikuMikuWorld
{
	constexpr const char* clipboardSignature = "MikuMikuWorld clipboard\n";

	void ScoreContext::setStep(HoldStepType type)
	{
		if (selectedNotes.empty())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selectedNotes)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() != NoteType::HoldMid)
				continue;

			HoldNote& hold = score.holdNotes.at(note.parentID);
			if (hold.isGuide())
				continue;

			int pos = findHoldStep(hold, id);
			if (pos != -1)
			{
				if (type == HoldStepType::HoldStepTypeCount)
				{
					cycleStepType(hold.steps[pos]);
					edit = true;
				}
				else
				{
					// don't record history if the type did not change
					edit |= hold.steps[pos].type != type;
					hold.steps[pos].type = type;
				}
			}
		}

		if (edit)
			pushHistory("Change step type", prev, score);
	}

	void ScoreContext::setFlick(FlickType flick)
	{
		if (selectedNotes.empty())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			bool canFlick = note.canFlick();

			if (note.getType() == NoteType::HoldEnd)
			{
				canFlick = score.holdNotes.at(note.parentID).endType == HoldNoteType::Normal;
			}

			if (canFlick)
			{
				if (flick == FlickType::FlickTypeCount)
				{
					cycleFlick(note);
					edit = true;
				}
				else
				{
					edit |= note.flick != flick;
					note.flick = flick;
				}
			}
		}

		if (edit)
			pushHistory("Change flick", prev, score);
	}

	void ScoreContext::setEase(EaseType ease)
	{
		if (selectedNotes.empty())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				if (ease == EaseType::EaseTypeCount)
				{
					cycleStepEase(score.holdNotes.at(note.ID).start);
					edit = true;
				}
				else
				{
					edit |= score.holdNotes.at(note.ID).start.ease != ease;
					score.holdNotes.at(note.ID).start.ease = ease;
				}
			}
			else if (note.getType() == NoteType::HoldMid)
			{
				HoldNote& hold = score.holdNotes.at(note.parentID);
				int pos = findHoldStep(hold, id);
				if (pos != -1)
				{
					if (ease == EaseType::EaseTypeCount)
					{
						cycleStepEase(hold.steps[pos]);
						edit = true;
					}
					else
					{
						// don't record history if the type did not change
						edit |= hold.steps[pos].ease != ease;
						hold.steps[pos].ease = ease;
					}
				}
			}
		}

		if (edit)
			pushHistory("Change ease", prev, score);
	}

	void ScoreContext::setHoldType(HoldNoteType hold)
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		bool edit = false;
		for (int id : selectedNotes)
		{
			// Invisible hold points cannot be trace notes!
			Note& note = score.notes.at(id);
			if (!(note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd))
				continue;
			HoldNote& holdNote = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);

			// For now do not allow changing guides to normal holds or vice versa
			if (holdNote.isGuide())
				continue;

			if (note.getType() == NoteType::Hold)
			{
				if ((hold != HoldNoteType::Normal))
					note.friction = false;

				holdNote.startType = hold;
				edit = true;
			}
			else if (note.getType() == NoteType::HoldEnd)
			{
				// reset flick to none if the end is not normal
				if (hold != HoldNoteType::Normal)
				{
					note.flick = FlickType::None;
					note.friction = false;
				}

				holdNote.endType = hold;
				edit = true;
			}
		}

		if (edit)
			pushHistory("Change hold", prev, score);
	}

	void ScoreContext::setFadeType(FadeType fade)
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		bool edit = false;
		for (int id : selectedNotes)
		{
			// Invisible hold points cannot be trace notes!
			Note& note = score.notes.at(id);

			if (!(note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd))
				continue;
			HoldNote& holdNote = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);

			if (!holdNote.isGuide())
				continue;

			holdNote.fadeType = fade;
			edit = true;
		}

		if (edit)
			pushHistory("Change fade", prev, score);
	}

	void ScoreContext::setGuideColor(GuideColor color)
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		bool edit = false;
		for (int id : selectedNotes)
		{
			// Invisible hold points cannot be trace notes!
			Note& note = score.notes.at(id);

			if (!(note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd))
				continue;
			HoldNote& holdNote = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);

			if (!holdNote.isGuide())
				continue;

			if (color == GuideColor::GuideColorCount) {
				holdNote.guideColor = (GuideColor)(((int)holdNote.guideColor + 1) % (int)GuideColor::GuideColorCount);
				edit = true;
			} else {
				if (holdNote.guideColor != color) {
					holdNote.guideColor = color;
					edit = true;
				}
			}
		}

		if (edit)
			pushHistory("Change guide", prev, score);
	}

	void ScoreContext::setLayer(int layer)
	{
		if (selectedNotes.empty())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);

			if (note.layer == layer)
				continue;
			note.layer = layer;
			edit = true;
		}

		if (edit)
			pushHistory("Change layer", prev, score);
	}

	void ScoreContext::toggleCriticals()
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		std::unordered_set<int> critHolds;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Damage)
				// noop
			{
			}
			else if (note.getType() == NoteType::Tap)
			{
				note.critical ^= true;
			}
			else if (note.getType() == NoteType::HoldEnd && (note.isFlick() || note.friction))
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
			// flip critical state
			HoldNote& note = score.holdNotes.at(hold);

			if (note.isGuide()) {
				if (note.guideColor == GuideColor::Yellow) {
					note.guideColor = GuideColor::Green;
				} else {
					note.guideColor = GuideColor::Yellow;
				}
				continue;
			}
			bool critical = !score.notes.at(note.start.ID).critical;

			// again if the hold start is critical, every note in the hold must be critical
			score.notes.at(note.start.ID).critical = critical;
			score.notes.at(note.end).critical = critical;
			for (auto& step : note.steps)
				score.notes.at(step.ID).critical = critical;
		}

		pushHistory("Change critical note", prev, score);
	}

	void ScoreContext::toggleFriction()
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		bool edit = false;
		for (int id : selectedNotes)
		{
			// Hold steps and invisible hold points cannot be trace notes
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::HoldMid)
				continue;

			if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
			{
				HoldNote& holdNote = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);
				if (holdNote.isGuide())
					continue;

				if (note.getType() == NoteType::Hold)
				{
					holdNote.startType = HoldNoteType::Normal;
				}
				else
				{
					holdNote.endType = HoldNoteType::Normal;
					if (!note.isFlick() && note.friction && !score.notes.at(note.parentID).critical)
					{
						// Prevent critical hold end if the hold start is not critical
						note.critical = false;
					}
				}
			}

			note.friction = !note.friction;
			edit = true;
		}

		if (edit)
			pushHistory("Change trace notes", prev, score);
	}

	void ScoreContext::deleteSelection()
	{
		if (selectedNotes.empty())
			return;

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
					// find hold step and remove it from the steps data container
					if (score.holdNotes.find(note.parentID) != score.holdNotes.end())
					{
						std::vector<HoldStep>& steps = score.holdNotes.at(note.parentID).steps;
						steps.erase(std::find_if(steps.cbegin(), steps.cend(), [id](const HoldStep& s)
							{ return s.ID == id; }));
					}
				}
				score.notes.erase(id);
			}
			else
			{
				const HoldNote& hold = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);
				score.notes.erase(hold.start.ID);
				score.notes.erase(hold.end);

				// hold steps cannot exist without a hold
				for (const auto& step : hold.steps)
					score.notes.erase(step.ID);

				score.holdNotes.erase(hold.start.ID);
			}
		}

		selectedNotes.clear();
		pushHistory("Delete notes", prev, score);
	}

	void ScoreContext::flipSelection()
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		for (int id : selectedNotes)
		{
			Note& note = score.notes.at(id);
			note.lane = MAX_LANE - note.lane - note.width + 1;

			if (note.flick == FlickType::Left)
				note.flick = FlickType::Right;
			else if (note.flick == FlickType::Right)
				note.flick = FlickType::Left;
		}

		pushHistory("Flip notes", prev, score);
	}

	void ScoreContext::cutSelection()
	{
		copySelection();
		deleteSelection();
	}

	void ScoreContext::copySelection()
	{
		if (selectedNotes.empty())
			return;

		int minTick = score.notes.at(*std::min_element(selectedNotes.begin(), selectedNotes.end(), [this](int id1, int id2)
		{
			return score.notes.at(id1).tick < score.notes.at(id2).tick;
		})).tick;

		json data = jsonIO::noteSelectionToJson(score, selectedNotes, minTick);

		std::string clipboard{ clipboardSignature };
		clipboard.append(data.dump());

		ImGui::SetClipboardText(clipboard.c_str());
	}

	void ScoreContext::cancelPaste()
	{
		pasteData.pasting = false;
	}

	void ScoreContext::doPasteData(const json& data, bool flip)
	{
		int baseId = 0;
		pasteData.notes.clear();
		pasteData.damages.clear();
		pasteData.holds.clear();

		if (jsonIO::arrayHasData(data, "notes"))
		{
			for (const auto& entry : data["notes"])
			{
				Note note = jsonIO::jsonToNote(entry, NoteType::Tap);
				note.ID = baseId++;
				note.layer = selectedLayer;

				pasteData.notes[note.ID] = note;
			}
		}

		if (jsonIO::arrayHasData(data, "damages"))
		{
			for (const auto& entry : data["damages"])
			{
				Note note = jsonIO::jsonToNote(entry, NoteType::Damage);
				note.ID = baseId++;
				note.layer = selectedLayer;

				pasteData.notes[note.ID] = note;
			}
		}

		if (jsonIO::arrayHasData(data, "holds"))
		{
			for (const auto& entry : data["holds"])
			{
				if (!jsonIO::keyExists(entry, "start") || !jsonIO::keyExists(entry, "end"))
					continue;

				Note start = jsonIO::jsonToNote(entry["start"], NoteType::Hold);
				start.ID = baseId++;
				start.layer = selectedLayer;
				pasteData.notes[start.ID] = start;

				Note end = jsonIO::jsonToNote(entry["end"], NoteType::HoldEnd);
				end.ID = baseId++;
				end.parentID = start.ID;
				end.critical = start.critical || ((end.isFlick() || end.friction) && end.critical);
				end.layer = selectedLayer;
				pasteData.notes[end.ID] = end;

				std::string startEase = jsonIO::tryGetValue<std::string>(entry["start"], "ease", "linear");

				HoldNote hold;
				hold.start = { start.ID, HoldStepType::Normal, (EaseType)findArrayItem(startEase.c_str(), easeTypes, arrayLength(easeTypes)) };
				hold.end = end.ID;
				for (int i = 0; i < arrayLength(fadeTypes); ++i) {
					if (entry["fade"] == fadeTypes[i]) {
						hold.fadeType = (FadeType)i;
						break;
					}
				}
				for (int i = 0; i < arrayLength(guideColors); ++i) {
					if (entry["guide"] == guideColors[i]) {
						hold.guideColor = (GuideColor)i;
						break;
					}
				}

				if (jsonIO::keyExists(entry, "steps"))
				{
					hold.steps.reserve(entry["steps"].size());
					for (const auto& step : entry["steps"])
					{
						Note mid = jsonIO::jsonToNote(step, NoteType::HoldMid);
						mid.critical = start.critical;
						mid.ID = baseId++;
						mid.parentID = start.ID;
						mid.layer = selectedLayer;
						pasteData.notes[mid.ID] = mid;

						std::string midType = jsonIO::tryGetValue<std::string>(step, "type", "normal");
						std::string midEase = jsonIO::tryGetValue<std::string>(step, "ease", "linear");
						int stepTypeIndex = findArrayItem(midType.c_str(), stepTypes, arrayLength(stepTypes));
						int easeTypeIndex = findArrayItem(midEase.c_str(), easeTypes, arrayLength(stepTypes));

						// maintain compatibility with old step type names
						if (stepTypeIndex == -1)
						{
							stepTypeIndex = 0;
							if (midType == "invisible") stepTypeIndex = 1;
							if (midType == "ignored") stepTypeIndex = 2;
						}

						// maintain compatibility with old ease type names
						if (easeTypeIndex == -1)
						{
							easeTypeIndex = 0;
							if (midEase == "in") easeTypeIndex = 1;
							if (midEase == "out") easeTypeIndex = 2;
						}

						hold.steps.push_back({ mid.ID, (HoldStepType)stepTypeIndex, (EaseType)easeTypeIndex });
					}
				}

				std::string startType = jsonIO::tryGetValue<std::string>(entry["start"], "type", "normal");
				std::string endType = jsonIO::tryGetValue<std::string>(entry["end"], "type", "normal");

				if (startType == "guide" || endType == "guide")
				{
					hold.startType = hold.endType = HoldNoteType::Guide;
					start.friction = end.friction = false;
					end.flick = FlickType::None;
				}
				else
				{
					if (startType == "hidden")
					{
						hold.startType = HoldNoteType::Hidden;
						start.friction = false;
					}

					if (endType == "hidden")
					{
						hold.endType = HoldNoteType::Hidden;
						end.friction = false;
						end.flick = FlickType::None;
					}
				}

				pasteData.holds[hold.start.ID] = hold;
			}
		}

		if (flip)
		{
			for (auto& [_, note] : pasteData.notes)
			{
				note.lane = MAX_LANE - note.lane - note.width + 1;

				if (note.flick == FlickType::Left)
					note.flick = FlickType::Right;
				else if (note.flick == FlickType::Right)
					note.flick = FlickType::Left;
			}
		}

		pasteData.pasting = !pasteData.notes.empty();
		if (pasteData.pasting)
		{
			// find the lane in which the cursor is in the middle of pasted notes
			int left = MAX_LANE + score.metadata.laneExtension;
			int right = MIN_LANE - score.metadata.laneExtension;
			int leftmostLane = MAX_LANE + score.metadata.laneExtension;
			int rightmostLane = MIN_LANE - score.metadata.laneExtension;
			for (const auto& [_, note] : pasteData.notes)
			{
				leftmostLane = std::min(leftmostLane, note.lane);
				rightmostLane = std::max(rightmostLane, note.lane + note.width - 1);
				left = std::min(left, note.lane + note.width);
				right = std::max(right, note.lane);
			}

			pasteData.minLaneOffset = MIN_LANE - score.metadata.laneExtension - leftmostLane;
			pasteData.maxLaneOffset = MAX_LANE + score.metadata.laneExtension - rightmostLane;
			pasteData.midLane = (left + right) / 2;
		}
	}

	void ScoreContext::confirmPaste()
	{
		Score prev = score;

		// update IDs and copy notes
		for (auto& [_, note] : pasteData.notes)
		{
			note.ID += nextID;
			if (note.parentID != -1)
				note.parentID += nextID;

			note.lane += pasteData.offsetLane;
			note.tick += pasteData.offsetTicks;
			score.notes[note.ID] = note;
		}

		for (auto& [_, hold] : pasteData.holds)
		{
			hold.start.ID += nextID;
			hold.end += nextID;
			for (auto& step : hold.steps)
				step.ID += nextID;

			score.holdNotes[hold.start.ID] = hold;
		}

		// select newly pasted notes
		selectedNotes.clear();
		std::transform(pasteData.notes.begin(), pasteData.notes.end(),
			std::inserter(selectedNotes, selectedNotes.end()),
			[this](const auto& it) { return it.second.ID; });

		nextID += pasteData.notes.size();
		pasteData.pasting = false;
		pushHistory("Paste notes", prev, score);
	}

	void ScoreContext::paste(bool flip)
	{
		const char* clipboardDataPtr = ImGui::GetClipboardText();
		if (clipboardDataPtr == nullptr)
			return;

		std::string clipboardData(clipboardDataPtr);
		if (!startsWith(clipboardData, clipboardSignature))
			return;

		doPasteData(json::parse(clipboardData.substr(strlen(clipboardSignature))), flip);
	}

	void ScoreContext::shrinkSelection(Direction direction)
	{
		if (selectedNotes.size() < 2)
			return;

		Score prev = score;

		std::vector<int> sortedSelection(selectedNotes.begin(), selectedNotes.end());
		std::sort(sortedSelection.begin(), sortedSelection.end(), [this](int a, int b)
		{
			const Note& n1 = score.notes.at(a);
			const Note& n2 = score.notes.at(b);
			return n1.tick == n2.tick ? n1.lane < n2.lane : n1.tick < n2.tick;
		});

		int factor = 1; // tick increment/decrement amount
		if (direction == Direction::Up)
		{
			// start from the last note
			std::reverse(sortedSelection.begin(), sortedSelection.end());
			factor = -1;
		}

		int firstTick = score.notes.at(*sortedSelection.begin()).tick;
		for (int i = 0; i < sortedSelection.size(); ++i)
			score.notes[sortedSelection[i]].tick = firstTick + (i * factor);

		const std::unordered_set<int> holds = getHoldsFromSelection();
		for (const auto& hold : holds)
			sortHoldSteps(score, score.holdNotes.at(hold));

		pushHistory("Shrink notes", prev, score);
	}

	void ScoreContext::connectHoldsInSelection()
	{
		if (!selectionCanConnect())
			return;

		Score prev = score;
		Note& note1 = score.notes[*selectedNotes.begin()];
		Note& note2 = score.notes[*std::next(selectedNotes.begin())];

		// Determine correct order of notes
		Note& earlierNote = note1.getType() == NoteType::HoldEnd ? note1 : note2;
		Note& laterNote = note1.getType() == NoteType::HoldEnd ? note2 : note1;

		HoldNote& earlierHold = score.holdNotes[earlierNote.parentID];
		HoldNote& laterHold = score.holdNotes[laterNote.ID];

		// Connect both ends
		earlierHold.end = laterHold.end;
		laterNote.parentID = earlierHold.start.ID;

		// We need to determine whether the new end will be critical
		Note& earlierHoldStart = score.notes.at(earlierHold.start.ID);
		Note& laterHoldEnd = score.notes.at(score.holdNotes.at(laterNote.ID).end);
		laterHoldEnd.critical = earlierHoldStart.critical ? true : laterHoldEnd.isFlick() && laterHoldEnd.critical;

		// Update later note's end parent ID
		laterHoldEnd.parentID = earlierHold.start.ID;

		// Copy over later note's steps
		for (auto& step : laterHold.steps)
		{
			if (earlierHold.isGuide()) {
				step.type = HoldStepType::Hidden;
			}
			earlierHold.steps.push_back(step);

			Note& note = score.notes.at(step.ID);
			note.critical = earlierHoldStart.critical;
			note.parentID = earlierHold.start.ID;
		}

		// Create new steps to connect both ends
		Note earlierNoteAsMid = Note(NoteType::HoldMid, earlierNote.tick, earlierNote.lane, earlierNote.width);
		earlierNoteAsMid.ID = nextID++;
		earlierNoteAsMid.critical = earlierHoldStart.critical;
		earlierNoteAsMid.parentID = earlierHold.start.ID;

		Note laterNoteAsMid = Note(NoteType::HoldMid, laterNote.tick, laterNote.lane, laterNote.width);
		laterNoteAsMid.ID = nextID++;
		laterNoteAsMid.critical = earlierHoldStart.critical;
		laterNoteAsMid.parentID = earlierHold.start.ID;

		// Insert new steps to their appropriate containers
		score.notes[earlierNoteAsMid.ID] = earlierNoteAsMid;
		score.notes[laterNoteAsMid.ID] = laterNoteAsMid;
		earlierHold.steps.push_back({ earlierNoteAsMid.ID, earlierHold.isGuide() ? HoldStepType::Hidden : HoldStepType::Normal, EaseType::Linear });
		earlierHold.steps.push_back({ laterNoteAsMid.ID, earlierHold.isGuide() ? HoldStepType::Hidden : laterHold.start.type, laterHold.start.ease });

		// Remove old notes
		score.notes.erase(earlierNote.ID);
		score.notes.erase(laterNote.ID);
		score.holdNotes.erase(laterHold.start.ID);

		sortHoldSteps(score, earlierHold);

		selectedNotes.clear();
		selectedNotes.insert(earlierNoteAsMid.ID);
		selectedNotes.insert(laterNoteAsMid.ID);

		pushHistory("Connect holds", prev, score);
	}

	void ScoreContext::splitHoldInSelection()
	{
		if (selectedNotes.size() != 1)
			return;

		Score prev = score;

		Note& note = score.notes[*selectedNotes.begin()];
		if (note.getType() != NoteType::HoldMid)
			return;

		HoldNote& hold = score.holdNotes[note.parentID];

		int pos = findHoldStep(hold, note.ID);
		if (pos == -1)
			return;

		Note holdStart = score.notes.at(hold.start.ID);

		Note newSlideEnd = Note(NoteType::HoldEnd, note.tick, note.lane, note.width);
		newSlideEnd.ID = nextID++;
		newSlideEnd.parentID = hold.start.ID;
		newSlideEnd.critical = note.critical;
		newSlideEnd.layer = holdStart.layer;

		Note newSlideStart = Note(NoteType::Hold, note.tick, note.lane, note.width);
		newSlideStart.ID = nextID++;
		newSlideStart.critical = holdStart.critical;
		newSlideStart.layer = holdStart.layer;

		HoldNote newHold;
		newHold.end = hold.end;

		Note& slideEnd = score.notes.at(hold.end);
		slideEnd.parentID = newSlideStart.ID;

		hold.end = newSlideEnd.ID;
		newHold.start = { newSlideStart.ID, HoldStepType::Normal, hold.steps[pos].ease };
		newHold.startType = hold.startType;
		newHold.endType = hold.endType;
		newHold.fadeType = hold.fadeType;
		newHold.guideColor = hold.guideColor;

		// Backwards loop to avoid incorrect indices after removal
		for (int i = hold.steps.size() - 1; i > pos; i--)
		{
			HoldStep& step = hold.steps[i];
			Note& stepNote = score.notes.at(step.ID);
			stepNote.parentID = newSlideStart.ID;
			newHold.steps.push_back(step);
			hold.steps.erase(hold.steps.begin() + i);
		}

		hold.steps.pop_back();
		score.notes.erase(note.ID);

		sortHoldSteps(score, hold);
		sortHoldSteps(score, newHold);
		selectedNotes.clear();
		selectedNotes.insert(newSlideStart.ID);
		selectedNotes.insert(newSlideEnd.ID);

		score.notes[newSlideEnd.ID] = newSlideEnd;
		score.notes[newSlideStart.ID] = newSlideStart;
		score.holdNotes[newSlideStart.ID] = newHold;
		pushHistory("Split hold", prev, score);
	}

	void ScoreContext::undo()
	{
		if (history.hasUndo())
		{
			score = history.undo();
			clearSelection();

			UI::setWindowTitle((workingData.filename.size() ? File::getFilename(workingData.filename) : windowUntitled) + "*");
			upToDate = false;

			scoreStats.calculateStats(score);
		}
	}

	void ScoreContext::redo()
	{
		if (history.hasRedo())
		{
			score = history.redo();
			clearSelection();

			UI::setWindowTitle((workingData.filename.size() ? File::getFilename(workingData.filename) : windowUntitled) + "*");
			upToDate = false;

			scoreStats.calculateStats(score);
		}
	}

	void ScoreContext::pushHistory(std::string description, const Score& prev, const Score& curr)
	{
		history.pushHistory(description, prev, curr);

		UI::setWindowTitle((workingData.filename.size() ? File::getFilename(workingData.filename) : windowUntitled) + "*");
		scoreStats.calculateStats(score);

		upToDate = false;
	}

	bool ScoreContext::selectionHasEase() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
			[this](const int id) { return score.notes.at(id).hasEase(); });
	}

	bool ScoreContext::selectionHasStep() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
			[this](const int id) { return score.notes.at(id).getType() == NoteType::HoldMid; });
	}

	bool ScoreContext::selectionHasFlickable() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
			[this](const int id) { return score.notes.at(id).canFlick(); });
	}

	bool ScoreContext::selectionCanConnect() const
	{
		if (selectedNotes.size() != 2)
			return false;

		const auto& note1 = score.notes.at(*selectedNotes.begin());
		const auto& note2 = score.notes.at(*std::next(selectedNotes.begin()));
		if (note1.tick == note2.tick)
			return (note1.getType() == NoteType::Hold && note2.getType() == NoteType::HoldEnd) ||
						 (note1.getType() == NoteType::HoldEnd && note2.getType() == NoteType::Hold);

		auto noteTickCompareFunc = [](const Note& n1, const Note& n2) { return n1.tick < n2.tick; };
		Note earlierNote = std::min(note1, note2, noteTickCompareFunc);
		Note laterNote = std::max(note1, note2, noteTickCompareFunc);

		return (earlierNote.getType() == NoteType::HoldEnd && laterNote.getType() == NoteType::Hold);
	}

	bool ScoreContext::selectionCanChangeHoldType() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(), [this](const int id)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
				return !score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID).isGuide();

			return false;
		});
	}

	bool ScoreContext::selectionCanChangeFadeType() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(), [this](const int id)
		{
			const Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold || note.getType() == NoteType::HoldEnd)
				return score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID).isGuide();

			return false;
		});
	}
}
